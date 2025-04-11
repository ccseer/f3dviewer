#include "sidebarwnd.h"

#include "ui_sidebarwnd.h"

SidebarWnd::SidebarWnd(QWidget *parent)
    : QWidget(parent), ui(new Ui::SidebarWnd)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    ui->scrollArea->setFrameStyle(QFrame::NoFrame);

    updateAnimationPlayBtnText();
    ui->pushButton_ani_reset->setVisible(false);

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
}

SidebarWnd::~SidebarWnd()
{
    delete ui;
}

void SidebarWnd::updateDPR(qreal r)
{
    setFixedWidth(r * 300);
    const auto list = {ui->label_camera_reset, ui->label_camera_x,
                       ui->label_camera_y, ui->label_camera_z};
    int w           = 0;
    for (auto i : list) {
        w = qMax(w, i->fontMetrics().horizontalAdvance(i->text()));
    }
    for (auto i : list) {
        i->setFixedWidth(w);
    }
}

void SidebarWnd::syncControls(bool grid,
                              bool edge,
                              bool ps,
                              bool meta,
                              bool fps,
                              bool ani_show_grp,
                              bool ani_running)
{
    ui->checkBox_display_grid->setChecked(grid);
    ui->checkBox_display_edge->setChecked(edge);
    ui->checkBox_display_ponit_sprites->setChecked(ps);
    ui->checkBox_display_metadata->setChecked(meta);
    ui->checkBox_display_fps->setChecked(fps);

    ui->widget_grp_animation->setVisible(ani_show_grp);
    m_ani_run = ani_running;
    updateAnimationPlayBtnText();
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
    constexpr auto g_ani_play  = "Play";
    constexpr auto g_ani_pause = "Pause";
    ui->pushButton_ani_play->setText(m_ani_run ? g_ani_pause : g_ani_play);
}
