#include "sidebarwnd.h"

#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextDocument>
#include <QToolButton>

#include "ui_sidebarwnd.h"

SidebarWnd::SidebarWnd(QWidget *parent)
    : QWidget(parent), ui(new Ui::SidebarWnd)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    ui->scrollArea->setFrameStyle(QFrame::NoFrame);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->verticalScrollBar()->setProperty("is_readonly", true);

    updateAnimationPlayBtnText();
    ui->pushButton_ani_reset->setVisible(false);
    ui->slider_ani_progress->setVisible(false);
    ui->label_ani_progress->setVisible(false);
    ui->label_ani_progress_val->setVisible(false);
    ui->slider_ani_speed->setVisible(false);
    ui->label_ani_speed->setVisible(false);
    ui->label_ani_speed_val->setVisible(false);
    ui->slider_ani_progress->setEnabled(false);
    ui->label_ani_progress_val->setText("0.0 / 0.0");
    ui->widget_keys_content->setVisible(false);
    ui->toolButton_keys_toggle->setAutoRaise(true);
    ui->toolButton_keys_toggle->setStyleSheet(
        "QToolButton {"
        " color: palette(midlight);"
        " font-size: 11px;"
        " padding: 1px 6px;"
        " border: 1px solid palette(mid);"
        " border-radius: 8px;"
        " background: transparent;"
        " }"
        "QToolButton:hover {"
        " color: palette(text);"
        " border-color: palette(midlight);"
        " }"
        "QToolButton:checked {"
        " color: palette(text);"
        " border-color: palette(buttontext);"
        " }"
        "QToolButton:pressed {"
        " border: none;"
        " padding: 1px 6px;"
        " border: 1px solid palette(buttontext);"
        " }");
    ui->toolButton_keys_toggle->setText("Show");

    initKeys();
    renderKeys(devicePixelRatioF());

    connect(ui->pushButton_camera_reset, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraReset);
    connect(ui->pushButton_camera_font, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraFront);
    connect(ui->pushButton_camera_back, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraBack);
    connect(ui->pushButton_camera_left, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraLeft);
    connect(ui->pushButton_camera_right, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraRight);
    connect(ui->pushButton_camera_top, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraTop);
    connect(ui->pushButton_camera_btm, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraBottom);

    connect(ui->checkBox_display_axis, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowAxis);
    connect(ui->checkBox_display_grid, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowGrid);
    connect(ui->checkBox_display_edge, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowEdge);
    connect(ui->checkBox_display_ponit_sprites, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowPointSprites);
    connect(ui->checkBox_display_fps, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowFPS);
    connect(ui->checkBox_display_metadata, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowMetadata);

    connect(ui->slider_ani_speed, &QSlider::valueChanged, this,
            [this](int value) {
                double speed = value / 100.0;
                ui->label_ani_speed_val->setText(
                    QString("%1x").arg(speed, 0, 'f', 1));
                emit sigAnimationSpeedChanged(speed);
            });
    connect(ui->toolButton_keys_toggle, &QToolButton::toggled, this,
            [this](bool expanded) {
                ui->widget_keys_content->setVisible(expanded);
                ui->toolButton_keys_toggle->setText(expanded ? "Hide" : "Show");
            });
}

SidebarWnd::~SidebarWnd()
{
    delete ui;
}

void SidebarWnd::updateDPR(qreal r)
{
    setFixedWidth(r * 240);
    const auto list = {ui->label_camera_reset, ui->label_camera_x,
                       ui->label_camera_y, ui->label_camera_z};
    int w           = 0;
    for (auto i : list) {
        w = qMax(w, i->fontMetrics().horizontalAdvance(i->text()));
    }
    for (auto i : list) {
        i->setFixedWidth(w);
    }
    auto font = qApp->font();
    font.setPixelSize(14 * r);
    font.setBold(true);
    for (auto i : findChildren<QLabel *>()) {
        if (i->objectName().startsWith("label_title_")) {
            i->setFont(font);
        }
    }

    w      = r * 75;
    auto h = r * 25;
    for (auto i : findChildren<QPushButton *>()) {
        i->setFixedSize(w, h);
    }

    const int ani_label_w
        = qMax(ui->label_ani_progress->fontMetrics().horizontalAdvance(
                   ui->label_ani_progress->text()),
               ui->label_ani_speed->fontMetrics().horizontalAdvance(
                   ui->label_ani_speed->text()));
    ui->label_ani_progress->setFixedWidth(ani_label_w);
    ui->label_ani_speed->setFixedWidth(ani_label_w);

    const int ani_value_w = qMax(
        ui->label_ani_progress_val->fontMetrics().horizontalAdvance(
            "99.9 / 99.9"),
        ui->label_ani_speed_val->fontMetrics().horizontalAdvance("999.9x"));
    ui->label_ani_progress_val->setFixedWidth(ani_value_w);
    ui->label_ani_speed_val->setFixedWidth(ani_value_w);

    auto smallFont = qApp->font();
    smallFont.setPixelSize(qRound(12 * r));
    ui->toolButton_keys_toggle->setFont(smallFont);
    renderKeys(r);

    QTextDocument doc;
    doc.setDefaultFont(ui->label_keys->font());
    doc.setHtml(ui->label_keys->text());
    ui->label_keys->setMinimumHeight(qRound(doc.size().height()));
}

void SidebarWnd::syncControls(bool axis,
                              bool grid,
                              bool edge,
                              bool ps,
                              bool meta,
                              bool fps,
                              bool ani_show_grp,
                              bool ani_running,
                              double ani_speed)
{
    ui->checkBox_display_axis->setChecked(axis);
    ui->checkBox_display_grid->setChecked(grid);
    ui->checkBox_display_edge->setChecked(edge);
    ui->checkBox_display_ponit_sprites->setChecked(ps);
    ui->checkBox_display_metadata->setChecked(meta);
    ui->checkBox_display_fps->setChecked(fps);

    ui->widget_grp_animation->setVisible(ani_show_grp);
    ui->slider_ani_progress->setVisible(ani_show_grp);
    ui->label_ani_progress->setVisible(ani_show_grp);
    ui->label_ani_progress_val->setVisible(ani_show_grp);
    ui->slider_ani_speed->setVisible(ani_show_grp);
    ui->label_ani_speed->setVisible(ani_show_grp);
    ui->label_ani_speed_val->setVisible(ani_show_grp);
    {
        const QSignalBlocker blocker(ui->slider_ani_speed);
        ui->slider_ani_speed->setValue(qRound(ani_speed * 100.0));
    }
    ui->label_ani_speed_val->setText(QString("%1x").arg(ani_speed, 0, 'f', 1));
    m_ani_run = ani_running;
    updateAnimationPlayBtnText();
}

void SidebarWnd::updateAnimationProgress(double current, double duration)
{
    const int max = qMax(1, qRound(duration * 1000.0));
    const int val = qBound(0, qRound(current * 1000.0), max);
    {
        const QSignalBlocker blocker(ui->slider_ani_progress);
        ui->slider_ani_progress->setRange(0, max);
        ui->slider_ani_progress->setValue(val);
    }
    ui->label_ani_progress_val->setText(
        QString("%1 / %2").arg(current, 0, 'f', 1).arg(duration, 0, 'f', 1));
}

void SidebarWnd::on_pushButton_ani_play_clicked()
{
    m_ani_run = !m_ani_run;
    updateAnimationPlayBtnText();
    emit sigPlayAnimation(m_ani_run);
}

void SidebarWnd::on_pushButton_ani_reset_clicked()
{
    m_ani_run = false;
    updateAnimationPlayBtnText();
    emit sigResetAnimationPos();
}

void SidebarWnd::updateAnimationPlayBtnText()
{
    ui->pushButton_ani_play->setText(m_ani_run ? "Pause" : "Play");
}

void SidebarWnd::initKeys()
{
    ui->label_keys->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->label_keys->setWordWrap(true);
    ui->label_keys->setTextFormat(Qt::RichText);
}

void SidebarWnd::renderKeys(qreal dpr)
{
    const QVector<QPair<QString, QString>> mouse
        = {{"Left Button Drag", "Rotate camera"},
           {"Shift + Left Button Drag", "Fine rotation"},
           {"Ctrl + Left Button Drag", "Roll camera around view axis"},
           {"Right Button Drag", "Pan camera"},
           {"Shift + Right Button Drag", "Fine panning"},
           {"Mouse Wheel Scroll", "Zoom in or out"},
           {"Shift + Mouse Wheel Scroll", "Fine zoom"},
           {"Double Click Left Button", "Reset camera to default view"}};
    const QVector<QPair<QString, QString>> key
        = {{"1-6", "Switch camera views"},
           {"Ctrl+C", "Copy current view"},
           {"B", "Toggle scalar bar"},
           {"Ctrl+P", "Increase model transparency"},
           {"Shift+P", "Decrease model transparency"},
           {"P", "Toggle translucency effect"},
           {"A", "Toggle anti-aliasing"},
           {"Shift+A", "Toggle skeleton display"},
           {"T", "Toggle tone mapping"},
           {"E", "Toggle model edges"},
           {"G", "Toggle ground grid"},
           {"M", "Toggle metadata display"},
           {"Z", "Toggle FPS counter"},
           {"V", "Toggle volume rendering"},
           {"I", "Invert volume rendering"},
           {"O", "Toggle point sprites"},
           {"U", "Toggle background blur"},
           {"Q", "Toggle ambient occlusion"},
           {"F", "Toggle ambient lighting"},
           {"J", "Toggle skybox background"},
           {"L", "Increase light intensity"},
           {"Shift+L", "Decrease light intensity"},
           {"Enter", "Reset camera view"},
           {"Tab", "Toggle sidebar"}};

    const int section_px  = qRound(12 * dpr);
    const int key_px      = qRound(12 * dpr);
    const int body_px     = qRound(13 * dpr);
    const int intro_px    = qRound(12 * dpr);
    const int card_gap    = qMax(10, qRound(10 * dpr));
    const int line_gap    = qMax(1, qRound(1 * dpr));
    const int section_gap = qMax(8, qRound(8 * dpr));
    const int intro_gap   = qMax(8, qRound(8 * dpr));

    auto buildSection = [](const QString &title,
                           const QVector<QPair<QString, QString>> &items,
                           int section_px, int key_px, int body_px,
                           int card_gap, int line_gap) {
        QString html
            = QString(
                  "<div style='margin-top:2px; margin-bottom:4px;'>"
                  "<span style='font-size:%1px; letter-spacing:0.08em; "
                  "text-transform:uppercase; color:#b8b8b8;'>%2</span>"
                  "</div>"
                  "<table cellspacing='0' cellpadding='0' width='100%'>")
                  .arg(section_px)
                  .arg(title.toHtmlEscaped());
        for (const auto &item : items) {
            html.append(QString("<tr><td style='font-family:Consolas, "
                                "\"Cascadia Mono\", monospace; "
                                "font-size:%1px; color:#f4d58d;'>%2</td></tr>"
                                "<tr><td style='padding-top:%3px; "
                                "font-size:%4px; color:#d6d6d6;'>%5</td></tr>"
                                "<tr><td height='%6'></td></tr>")
                            .arg(key_px)
                            .arg(item.first.toHtmlEscaped())
                            .arg(line_gap)
                            .arg(body_px)
                            .arg(item.second.toHtmlEscaped())
                            .arg(card_gap));
        }
        html.append("</table>");
        return html;
    };

    QString text;
    text.append("<div style='line-height:1.2'>");
    text.append(
        QString(
            "<div style='margin-bottom:%1px; font-size:%2px; color:#a8a8a8;'>")
            .arg(intro_gap)
            .arg(intro_px));
    text.append(
        "Practical shortcuts for quick navigation and display control.");
    text.append("</div>");
    text.append(buildSection("Mouse", mouse, section_px, key_px, body_px,
                             card_gap, line_gap));
    text.append(QString("<div style='height:%1px'></div>").arg(section_gap));
    text.append("<br/>");
    text.append(buildSection("Keyboard", key, section_px, key_px, body_px,
                             card_gap, line_gap));
    text.append("</div>");

    ui->label_keys->setText(text);
    ui->label_keys->adjustSize();
}
