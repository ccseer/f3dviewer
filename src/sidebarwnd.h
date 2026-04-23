#pragma once

#include <QStringList>
#include <QWidget>

namespace Ui {
class SidebarWnd;
}

class SidebarWnd : public QWidget {
    Q_OBJECT

public:
    struct State {
        bool axis                = false;
        bool grid                = false;
        bool edge                = false;
        bool pointSprites        = false;
        bool scalarBar           = false;
        bool metadata            = false;
        bool fps                 = false;
        bool antiAliasing        = false;
        bool ambientOcclusion    = false;
        bool toneMapping         = false;
        bool translucencySupport = false;
        bool orthographic        = false;
        bool hdriAmbient         = false;
        bool skybox              = false;
        bool volumeRendering     = false;
        bool backgroundBlur      = false;
        bool animationVisible    = false;
        bool animationRunning    = false;
        bool animationLoop       = true;
        bool yUp                 = true;
        int animationSelection   = -1;
        int opacityPercent       = 100;
        double animationSpeed    = 1.0;
    };

    explicit SidebarWnd(QWidget *parent = nullptr);
    ~SidebarWnd() override;

    void updateDPR(qreal r);

    Q_SIGNAL void sigPlayAnimation(bool play);
    Q_SIGNAL void sigSeekAnimation(double seconds);
    Q_SIGNAL void sigAnimationSelectionChanged(int index);
    Q_SIGNAL void sigAnimationLoopChanged(bool loop);

    Q_SIGNAL void sigCameraReset();
    Q_SIGNAL void sigCameraFront();
    Q_SIGNAL void sigCameraBack();
    Q_SIGNAL void sigCameraLeft();
    Q_SIGNAL void sigCameraRight();
    Q_SIGNAL void sigCameraTop();
    Q_SIGNAL void sigCameraBottom();
    Q_SIGNAL void sigYUpChanged(bool yUp);
    Q_SIGNAL void sigOrthographicChanged(bool on);

    Q_SIGNAL void sigShowAxis(bool);
    Q_SIGNAL void sigShowGrid(bool);
    Q_SIGNAL void sigShowEdge(bool);
    Q_SIGNAL void sigShowPointSprites(bool);
    Q_SIGNAL void sigShowScalarBar(bool);
    Q_SIGNAL void sigShowMetadata(bool);
    Q_SIGNAL void sigShowFPS(bool);
    Q_SIGNAL void sigShowAntiAliasing(bool);
    Q_SIGNAL void sigShowAmbientOcclusion(bool);
    Q_SIGNAL void sigShowToneMapping(bool);
    Q_SIGNAL void sigShowTranslucencySupport(bool);
    Q_SIGNAL void sigShowHdriAmbient(bool);
    Q_SIGNAL void sigShowSkybox(bool);
    Q_SIGNAL void sigShowVolumeRendering(bool);
    Q_SIGNAL void sigShowBackgroundBlur(bool);
    Q_SIGNAL void sigOpacityChanged(double opacity);
    Q_SIGNAL void sigAnimationSpeedChanged(double speed);
    Q_SIGNAL void sigResetViewOptions();

    void syncControls(const State &state);
    void setAnimationList(const QStringList &names, int currentIndex);
    void updateAnimationProgress(double current, double duration);

private:
    Q_SLOT void on_pushButton_ani_play_clicked();
    void updateAnimationPlayBtnText();
    void updateCameraAxisLabels(bool yUp);

    void initKeys();
    void renderKeys(qreal dpr);

    bool m_ani_run = true;
    bool m_syncing = false;

    Ui::SidebarWnd *ui;
};
