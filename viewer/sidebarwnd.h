#pragma once

#include <QWidget>

namespace Ui {
class SidebarWnd;
}

class SidebarWnd : public QWidget {
    Q_OBJECT

public:
    explicit SidebarWnd(QWidget *parent = nullptr);
    ~SidebarWnd() override;

    void updateDPR(qreal r);

    Q_SIGNAL void sigPlayAnimation(bool play);
    Q_SIGNAL void sigResetAnimationPos();

    Q_SIGNAL void sigCameraReset();
    Q_SIGNAL void sigCameraFront();
    Q_SIGNAL void sigCameraBack();
    Q_SIGNAL void sigCameraLeft();
    Q_SIGNAL void sigCameraRight();
    Q_SIGNAL void sigCameraTop();
    Q_SIGNAL void sigCameraBottom();

    Q_SIGNAL void sigShowGrid(bool);
    Q_SIGNAL void sigShowEdge(bool);
    Q_SIGNAL void sigShowPointSprites(bool);
    Q_SIGNAL void sigShowMetadata(bool);
    Q_SIGNAL void sigShowFPS(bool);

    void syncControls(bool grid,
                      bool edge,
                      bool ps,
                      bool meta,
                      bool fps,
                      bool ani_show_grp,
                      bool ani_running);

private slots:
    void on_pushButton_ani_play_clicked();
    void on_pushButton_ani_reset_clicked();
    void updateAnimationPlayBtnText();

private:
    bool m_ani_run = true;

    Ui::SidebarWnd *ui;
};
