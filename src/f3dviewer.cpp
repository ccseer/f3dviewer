#include "f3dviewer.h"

#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QPainter>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QTimer>
#include <QToolButton>

#include "f3dwidget/F3DWidget.h"
#include "seer/viewerhelper.h"
#include "sidebarwnd.h"

#define qprintt qDebug() << "[F3DViewer]"

namespace {
constexpr auto g_ini_sidebar_visible = "sidebar_visible";
constexpr auto g_ini_grid            = "display_grid";
constexpr auto g_ini_axis            = "display_axis";
constexpr auto g_ini_edge            = "display_edge";
constexpr auto g_ini_metadata        = "display_metadata";
constexpr auto g_ini_fps             = "display_fps";

constexpr auto g_svg_sidebar = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none"
     stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
  <rect x="3" y="3" width="18" height="18" rx="2"/>
  <line x1="9" y1="3" x2="9" y2="21"/>
</svg>)SVG";

QIcon svgIcon(const char *svg_data, bool dark_theme, qreal dpr)
{
    QByteArray data(svg_data);
    const QByteArray color = dark_theme ? "#e0e0e0" : "#303030";
    data.replace("currentColor", color);
    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }
    constexpr int icon_sz = 20;
    const int phys        = qRound(icon_sz * dpr);
    QPixmap pix(phys, phys);
    pix.setDevicePixelRatio(dpr);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    renderer.render(&p, QRectF(0, 0, icon_sz, icon_sz));
    return QIcon(pix);
}
}  // namespace

F3DViewer::F3DViewer(QWidget *parent) : ViewerBase(parent)
{
    qprintt << this;
    m_ini = new QSettings(getIniPath(), QSettings::IniFormat, this);
    if (m_ini) {
        m_sidebar_visible_pref
            = m_ini->value(g_ini_sidebar_visible, true).toBool();
    }
}

F3DViewer::~F3DViewer()
{
    saveIni();
    qprintt << "~" << this;
}

void F3DViewer::saveIni()
{
    if (!m_ini) {
        return;
    }
    m_ini->setValue(g_ini_sidebar_visible, m_sidebar_visible_pref);
    if (m_view) {
        m_ini->setValue(g_ini_grid,
                        m_view->getOption("render.grid.enable").toBool());
        m_ini->setValue(g_ini_axis, m_view->getOption("ui.axis").toBool());
        m_ini->setValue(g_ini_edge,
                        m_view->getOption("render.show_edges").toBool());
        m_ini->setValue(g_ini_metadata,
                        m_view->getOption("ui.metadata").toBool());
        m_ini->setValue(g_ini_fps, m_view->getOption("ui.fps").toBool());
    }
    m_ini->sync();
}

QSize F3DViewer::getContentSize() const
{
    const auto sz_def = options()->dpr() * QSize{950, 700};
    auto cmd
        = options()->property(ViewOptionsKeys::kKeyPluginCmd).toStringList();
    if (!cmd.isEmpty()) {
        auto parsed = seer::parseViewerSizeFromConfig(cmd);
        qprintt << "getContentSize: parsed" << parsed << cmd;
        if (parsed.isValid()) {
            return parsed;
        }
    }
    return sz_def;
}

void F3DViewer::updateDPR(qreal r)
{
    m_sidebar->updateDPR(r);
    const int sz = qRound(30 * r);
    m_btn->setFixedSize(sz, sz);
    m_btn->setIconSize(QSize(sz - 10, sz - 10));
    if (m_view) {
        m_view->setUIScale(r);
    }
}

void F3DViewer::updateTheme(int t)
{
    const bool dark = (t != 0);
    if (m_btn) {
        const auto icon = svgIcon(g_svg_sidebar, dark, options()->dpr());
        m_btn->setIcon(icon);
        m_btn->setText(icon.isNull() ? "SB" : QString());
    }
    if (m_view) {
        const auto &bg = qApp->palette().color(QPalette::Window);
        m_view->setOption("render.background.color", QString("%1,%2,%3")
                                                         .arg(bg.redF())
                                                         .arg(bg.greenF())
                                                         .arg(bg.blueF()));
    }
}

void F3DViewer::keyPressEvent(QKeyEvent *event)
{
    // ViewerBase::keyPressEvent(event);
    if (event->key() == Qt::Key_Tab) {
        setSidebarVisible(!m_sidebar->isVisible());
        return;
    }
    if (m_view) {
        qApp->sendEvent(m_view, event);
        syncSidebar();
    }
}

void F3DViewer::loadImpl(QBoxLayout *lay_content, QHBoxLayout *lay_ctrlbar)
{
    m_view = new F3DWidget(this);
    initSidebar();
    QHBoxLayout *hbl = new QHBoxLayout();
    hbl->setContentsMargins(0, 0, 0, 0);
    hbl->setSpacing(0);
    hbl->addWidget(m_view);
    hbl->addWidget(m_sidebar);

    lay_content->addLayout(hbl);
    if (!m_view->load(options()->path())) {
        emit sigCommand(VCT_StateChange, VCV_Error);
        return;
    }

    if (lay_ctrlbar) {
        lay_ctrlbar->addStretch();
        lay_ctrlbar->addWidget(m_btn);
    }
    else {
        m_btn->hide();
    }
    syncSidebar();
    updateTheme(options()->theme());
    updateDPR(options()->dpr());

    emit sigCommand(VCT_StateChange, VCV_Loaded);
}

void F3DViewer::initSidebar()
{
    m_sidebar = new SidebarWnd(this);
    m_btn     = new QToolButton(this);
    m_btn->setToolTip("Sidebar (Tab)");
    m_btn->setAutoRaise(true);
    m_btn->setFocusPolicy(Qt::NoFocus);

    auto *tab_shortcut = new QShortcut(QKeySequence(Qt::Key_Tab), this);
    tab_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(tab_shortcut, &QShortcut::activated, this,
            [this]() { setSidebarVisible(!m_sidebar->isVisible()); });

    connect(m_btn, &QToolButton::clicked, this,
            [this]() { setSidebarVisible(!m_sidebar->isVisible()); });

    // Apply INI display settings after engine+model are ready
    connect(m_view, &F3DWidget::sigLoaded, this, [this]() {
        if (!m_ini) {
            syncSidebar();
            QTimer::singleShot(0, this, [this]() {
                setSidebarVisible(m_sidebar_visible_pref);
            });
            return;
        }
        auto applyBool = [this](const char *iniKey, const char *opt, bool def) {
            bool v = m_ini->value(iniKey, def).toBool();
            m_view->setOption(opt, v ? "1" : "0");
        };
        applyBool(g_ini_grid, "render.grid.enable", true);
        applyBool(g_ini_axis, "ui.axis", true);
        applyBool(g_ini_edge, "render.show_edges", false);
        applyBool(g_ini_metadata, "ui.metadata", false);
        applyBool(g_ini_fps, "ui.fps", false);
        // plugin.json args override INI
        auto cmd = options()->property(ViewOptionsKeys::kKeyPluginCmd).toStringList();
        if (!cmd.isEmpty()) {
            m_view->applyOptions(cmd);
        }
        syncSidebar();
        QTimer::singleShot(
            0, this, [this]() { setSidebarVisible(m_sidebar_visible_pref); });
    });
    connect(m_view, &F3DWidget::sigAnimationStateChanged, this,
            [this](bool) { syncSidebar(); });
    connect(m_view, &F3DWidget::sigAnimationProgressChanged, m_sidebar,
            [this](double current, double duration) {
                m_sidebar->updateAnimationProgress(current, duration);
            });

    //
    connect(m_sidebar, &SidebarWnd::sigPlayAnimation, this,
            [this](bool play) { m_view->setAnimationState(play); });

    connect(m_sidebar, &SidebarWnd::sigShowAxis, this, [this](bool on) {
        m_view->setOption("ui.axis", on ? "1" : "0");
        if (m_ini) {
            m_ini->setValue(g_ini_axis, on);
        }
    });

    //
    connect(m_sidebar, &SidebarWnd::sigCameraReset, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Default); });
    connect(m_sidebar, &SidebarWnd::sigCameraFront, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Front); });
    connect(m_sidebar, &SidebarWnd::sigCameraBack, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Back); });
    connect(m_sidebar, &SidebarWnd::sigCameraLeft, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Left); });
    connect(m_sidebar, &SidebarWnd::sigCameraRight, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Right); });
    connect(m_sidebar, &SidebarWnd::sigCameraTop, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Top); });
    connect(m_sidebar, &SidebarWnd::sigCameraBottom, this,
            [this]() { m_view->moveCamera(F3DWidget::CP_Bottom); });

    //
    connect(m_sidebar, &SidebarWnd::sigShowGrid, this, [this](bool on) {
        m_view->setOption("render.grid.enable", on ? "1" : "0");
        if (m_ini) {
            m_ini->setValue(g_ini_grid, on);
        }
    });
    connect(m_sidebar, &SidebarWnd::sigShowEdge, this, [this](bool on) {
        m_view->setOption("render.show_edges", on ? "1" : "0");
        if (m_ini) {
            m_ini->setValue(g_ini_edge, on);
        }
    });
    connect(m_sidebar, &SidebarWnd::sigShowPointSprites, this, [this](bool on) {
        m_view->setOption("model.point_sprites.enable", on ? "1" : "0");
    });
    connect(m_sidebar, &SidebarWnd::sigShowMetadata, this, [this](bool on) {
        m_view->setOption("ui.metadata", on ? "1" : "0");
        if (m_ini) {
            m_ini->setValue(g_ini_metadata, on);
        }
    });
    connect(m_sidebar, &SidebarWnd::sigShowFPS, this, [this](bool on) {
        m_view->setOption("ui.fps", on ? "1" : "0");
        if (m_ini) {
            m_ini->setValue(g_ini_fps, on);
        }
    });

    connect(m_sidebar, &SidebarWnd::sigAnimationSpeedChanged, this,
            [this](double speed) { m_view->setAnimationSpeed(speed); });

    // Restore sidebar visibility from ini
    setSidebarVisible(m_sidebar_visible_pref);
}

void F3DViewer::syncSidebar()
{
    if (!m_view) {
        return;
    }
    qprintt << "syncSidebar" << m_view;

    m_sidebar->syncControls(
        m_view->getOption("ui.axis").toBool(),
        m_view->getOption("render.grid.enable").toBool(),
        m_view->getOption("render.show_edges").toBool(),
        m_view->getOption("model.point_sprites.enable").toBool(),
        m_view->getOption("ui.metadata").toBool(),
        m_view->getOption("ui.fps").toBool(), m_view->hasAnimation(),
        m_view->isAnimationRunning(), m_view->getAnimationSpeed());
    m_sidebar->updateAnimationProgress(m_view->getAnimationPosition(),
                                       m_view->getAnimationDuration());
}

void F3DViewer::setSidebarVisible(bool visible)
{
    if (!m_sidebar) {
        return;
    }
    m_sidebar_visible_pref = visible;
    m_sidebar->setVisible(visible);
    if (m_ini) {
        m_ini->setValue(g_ini_sidebar_visible, visible);
        m_ini->sync();
    }
}

QString F3DViewer::getIniPath() const
{
    const QString filename = name() % ".ini";
    QString dir            = QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation);
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(
            QStandardPaths::GenericDataLocation);
    }
    if (dir.isEmpty()) {
        dir = seer::getDLLPath();
    }
    if (dir.isEmpty()) {
        dir = QCoreApplication::applicationDirPath();
    }
    dir += "/Seer";
    dir.replace("\\", "/");
    QDir().mkpath(dir);
    if (!dir.endsWith("/")) {
        dir.append("/");
    }
    return dir + filename;
}
