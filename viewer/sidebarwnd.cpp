#include "sidebarwnd.h"

#include <QScrollBar>

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

void SidebarWnd::initKeys()
{
    ui->label_keys->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->label_keys->setWordWrap(true);

    QString text =
R"(Left Button Drag
    Rotate camera
Shift + Left Button Drag
    Fine rotation
Ctrl + Left Button Drag
    Roll camera (around view axis)
Right Button Drag
    Pan camera
Shift + Right Button Drag
    Fine panning
Mouse Wheel Scroll
    Zoom in/out
Shift + Mouse Wheel Scroll
    Fine zoom
Double Click Left Button
    Reset camera to default view

1-6
    Switch camera views
Ctrl+C
    Copy current view
B
    Toggle scalar bar
Ctrl+P
    Increase model transparency
Shift+P
    Decrease model transparency
P
    Toggle translucency effect
A
    Toggle anti-aliasing
Shift+A
    Toggle skeleton display
T
    Toggle tone mapping
E
    Toggle model edges
G
    Toggle ground grid
M
    Toggle metadata display
Z
    Toggle FPS counter
V
    Toggle volume rendering
I
    Invert volume rendering
O
    Toggle point sprites
U
    Toggle background blur
Q
    Toggle ambient occlusion
F
    Toggle ambient lighting
J
    Toggle skybox background
L
    Increase light intensity
Shift+L
    Decrease light intensity
Enter
    Reset camera view
Tab
    Toggle Sidebar)";
    ui->label_keys->setText(text);
}
