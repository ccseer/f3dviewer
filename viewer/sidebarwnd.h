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

    void hideAnimationGrp();
    Q_SIGNAL void sigPlayAnimation(bool play);
    Q_SIGNAL void sigResetAnimationPos();

private slots:
    void on_pushButton_ani_play_clicked(); 
    void on_pushButton_ani_reset_clicked();

private:
    bool m_ani_run = true;

    Ui::SidebarWnd *ui;
};
