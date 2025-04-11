#include "f3dviewer.h"

#include <QApplication>
#include <QKeyEvent>

#include "F3DWidget.h"
#include "sidebarwnd.h"

#define qprintt qDebug() << "[F3DViewer]"

F3DViewer::F3DViewer(QWidget* parent) : ViewerBase(parent)
{
    qprintt << this;
}

F3DViewer::~F3DViewer()
{
    qprintt << "~" << this;
}

QSize F3DViewer::getContentSize() const
{
    return m_d->d->dpr * QSize{950, 700};
}

void F3DViewer::updateDPR(qreal r)
{
    m_d->d->dpr = r;
    m_sidebar->setFixedWidth(r * 300);
    // m_view: "ui.scale";
}

void F3DViewer::updateTheme(int t)
{
    m_d->d->theme = t;
    if (m_view) {
        const auto& bg = qApp->palette().color(QPalette::Window);
        m_view->setOption("render.background.color", QString("%1,%2,%3")
                                                         .arg(bg.redF())
                                                         .arg(bg.greenF())
                                                         .arg(bg.blueF()));
    }
}

void F3DViewer::keyPressEvent(QKeyEvent* event)
{
    // ViewerBase::keyPressEvent(event);
    if (m_view) {
        qApp->sendEvent(m_view, event);
        syncSidebar();
    }
}

void F3DViewer::loadImpl(QBoxLayout* lay_content, QHBoxLayout* lay_ctrlbar)
{
    m_view = new F3DWidget(this);
    initSidebar();
    QHBoxLayout* hbl = new QHBoxLayout();
    hbl->setContentsMargins(0, 0, 0, 0);
    hbl->setSpacing(0);
    hbl->addWidget(m_view);
    hbl->addWidget(m_sidebar);

    lay_content->addLayout(hbl);
    if (!m_view->load(m_d->d->path)) {
        emit sigCommand(ViewCommandType::VCT_StateChange, VCV_Error);
        return;
    }
    if (lay_ctrlbar) {
        lay_ctrlbar->addStretch();
    }
    syncSidebar();
    updateTheme(m_d->d->theme);
    updateDPR(m_d->d->dpr);

    emit sigCommand(ViewCommandType::VCT_StateChange, VCV_Loaded);
}

void F3DViewer::initSidebar()
{
    m_sidebar = new SidebarWnd(this);

    //
    connect(m_sidebar, &SidebarWnd::sigPlayAnimation, this, [this](bool play) {
        m_view->setOption("animation.playing", play ? "1" : "0");
    });

    connect(m_sidebar, &SidebarWnd::sigResetAnimationPos, this,
            [this]() { m_view->setOption("animation.reset", "1"); });

    //
    // connect(m_sidebar, &SidebarWnd::sigCameraReset, this,
    //        [this]() { m_view->setOption("camera.resetPosition", "1"); });
    // connect(m_sidebar, &SidebarWnd::sigCameraFront, this,
    //        [this]() { m_view->setOption("camera.front", "1"); });
    // connect(m_sidebar, &SidebarWnd::sigCameraBack, this,
    //        [this]() { m_view->setOption("camera.back", "1"); });
    // connect(m_sidebar, &SidebarWnd::sigCameraLeft, this,
    //        [this]() { m_view->setOption("camera.left", "1"); });
    // connect(m_sidebar, &SidebarWnd::sigCameraRight, this,
    //        [this]() { m_view->setOption("camera.right", "1"); });
    // connect(m_sidebar, &SidebarWnd::sigCameraTop, this,
    //        [this]() { m_view->setOption("camera.top", "1"); });
    // connect(m_sidebar, &SidebarWnd::sigCameraBottom, this,
    //        [this]() { m_view->setOption("camera.bottom", "1"); });

    //
    connect(m_sidebar, &SidebarWnd::sigShowGrid, this, [this](bool on) {
        m_view->setOption("render.grid.enable", on ? "1" : "0");
    });
    connect(m_sidebar, &SidebarWnd::sigShowEdge, this, [this](bool on) {
        m_view->setOption("render.show_edges", on ? "1" : "0");
    });
    connect(m_sidebar, &SidebarWnd::sigShowPointSprites, this, [this](bool on) {
        m_view->setOption("model.point_sprites.enable", on ? "1" : "0");
    });
    connect(m_sidebar, &SidebarWnd::sigShowMetadata, this, [this](bool on) {
        m_view->setOption("ui.metadata", on ? "1" : "0");
    });
    connect(m_sidebar, &SidebarWnd::sigShowFPS, this,
            [this](bool on) { m_view->setOption("ui.fps", on ? "1" : "0"); });
}

void F3DViewer::syncSidebar()
{
    if (!m_view) {
        return;
    }
    qprintt << "syncSidebar" << m_view;

    m_sidebar->syncControls(
        m_view->getOption("render.grid.enable").toBool(),
        m_view->getOption("render.show_edges").toBool(),
        m_view->getOption("model.point_sprites.enable").toBool(),
        m_view->getOption("ui.metadata").toBool(),
        m_view->getOption("ui.fps").toBool(), m_view->hasAnimation());
}
