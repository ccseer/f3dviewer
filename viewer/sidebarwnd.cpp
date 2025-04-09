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
    this->layout()->setContentsMargins(0, 0, 0, 0);
    ui->pushButton_ani_play->setText(g_ani_play);
}

SidebarWnd::~SidebarWnd()
{
    delete ui;
}

void SidebarWnd::hideAnimationGrp()
{
    ui->widget_grp_animation->hide();
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
