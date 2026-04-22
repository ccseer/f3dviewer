#include "sidebarwnd.h"

#include <QScrollBar>
#include <QSignalBlocker>

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

    initKeys();

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

    const QMap<QString, QString> mouse
        = {{"Left Button Drag", "Rotate camera"},
           {"Shift + Left Button Drag", "Fine rotation"},
           {"Ctrl + Left Button Drag", "Roll camera (around view axis)"},
           {"Right Button Drag", "Pan camera"},
           {"Shift + Right Button Drag", "Fine panning"},
           {"Mouse Wheel Scroll", "Zoom in/out"},
           {"Shift + Mouse Wheel Scroll", "Fine zoom"},
           {"Double Click Left Button", "Reset camera to default view"}};
    const QMap<QString, QString> key
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

    constexpr auto sep = "\n";
    QString space      = "    ";
    QString text;
    for (const auto &i : mouse.keys()) {
        text.append(i).append(sep).append(space).append(mouse[i]);
        text.append("\n");
    }
    text.append("\n");
    for (const auto &i : key.keys()) {
        text.append(i).append(sep).append(space).append(key[i]);
        text.append("\n");
    }

    ui->label_keys->setText(text);
}
