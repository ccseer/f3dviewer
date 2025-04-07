
#pragma once

#include <QOpenGLWindow>
#include <QVector3D>
#include <memory>

namespace f3d {
class engine;
}

class F3DWindow : public QOpenGLWindow {
    Q_OBJECT
public:
    explicit F3DWindow(const QString& filePath);
    ~F3DWindow() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void handleKey(QKeyEvent* event);
    void moveCameraTo(const QVector3D& newPos,
                      const QVector3D& focal,
                      const QVector3D& up);

    std::unique_ptr<f3d::engine> m_engine;

    struct {
        QVector3D camera_pos_start;
        QVector3D camera_pos_end;
    } m_animation;

    QString m_path;
    QPointF m_pos;
};
