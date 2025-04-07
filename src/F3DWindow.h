
#pragma once

#include <QOpenGLWindow>
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

    std::unique_ptr<f3d::engine> m_engine;
    QString m_path;

    QPointF m_pos;
};
