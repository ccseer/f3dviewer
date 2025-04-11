#pragma once

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>

namespace f3d {
class engine;
}

class F3DWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit F3DWidget(QWidget* parent = nullptr);
    ~F3DWidget() override;

    bool load(const QString& path);

    void setOption(const QString& key, const QString& v);
    QVariant getOption(const QString& key) const;
    bool hasAnimation() const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void handleKey(QKeyEvent* event);
    void moveCameraTo(const QVector3D& new_pos,
                      const QVector3D& focal,
                      const QVector3D& up);
    void onAnimTick();

    struct {
        QElapsedTimer elapsed;
        QTimer timer;
        double speed = 1.;
        // for loadAnimationTime
        double pos   = 0;
        bool playing = true;
    } m_animation;

    std::unique_ptr<f3d::engine> m_engine;

    QPointF m_pos;
};
