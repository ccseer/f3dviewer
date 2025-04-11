#include "sidebarwnd.h"

#include "ui_sidebarwnd.h"

namespace {
constexpr auto g_ani_play  = "Play";
constexpr auto g_ani_pause = "Pause";
}  // namespace

SidebarWnd::SidebarWnd(QWidget *parent)
    : QWidget(parent), ui(new Ui::SidebarWnd)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    ui->scrollArea->setFrameStyle(QFrame::NoFrame);
    ui->pushButton_ani_play->setText(g_ani_play);

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

void SidebarWnd::syncControls(
    bool grid, bool edge, bool ps, bool meta, bool fps, bool show_ani)
{
    ui->checkBox_display_grid->setChecked(grid);
    ui->checkBox_display_edge->setChecked(edge);
    ui->checkBox_display_ponit_sprites->setChecked(ps);
    ui->checkBox_display_metadata->setChecked(meta);
    ui->checkBox_display_fps->setChecked(fps);

    ui->widget_grp_animation->setVisible(show_ani);
}

void SidebarWnd::on_pushButton_ani_play_clicked()
{
    m_ani_run = !m_ani_run;
    ui->pushButton_ani_play->setText(m_ani_run ? g_ani_play : g_ani_pause);
    emit sigPlayAnimation(m_ani_run);
}

void SidebarWnd::on_pushButton_ani_reset_clicked()
{
    m_ani_run = false;
    ui->pushButton_ani_play->setText(g_ani_pause);
    emit sigResetAnimationPos();
}
