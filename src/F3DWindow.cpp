#include "F3DWindow.h"

#include <f3d/engine.h>
#include <f3d/interactor.h>
#include <f3d/options.h>
#include <f3d/scene.h>
#include <f3d/window.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMouseEvent>
#include <QQuaternion>
#include <QVariantAnimation>
#include <QVector3D>

#define qprintt qDebug() << "[F3DWindow]"

namespace {
constexpr float g_zoom_factor  = 0.001f;
constexpr float g_zoom_speed   = 0.01f;
constexpr float g_rotate_speed = 0.5f;
}  // namespace

F3DWindow::F3DWindow(const QString& filePath)
    : QOpenGLWindow(), m_path(filePath)
{
    qprintt << this;
}

F3DWindow::~F3DWindow()
{
    qprintt << "~" << this;
    m_engine.reset();
    qprintt << "~" << this;
}

void F3DWindow::initializeGL()
{
    QOpenGLWindow::initializeGL();
    connect(this, &QOpenGLWindow::frameSwapped, this, [this]() {
        //
        update();
    });

    f3d::engine::autoloadPlugins();
    f3d::context::function ctx = [this](const char* name) {
        return this->context()->getProcAddress(name);
    };
    m_engine = std::make_unique<f3d::engine>(f3d::engine::createExternal(ctx));
    m_engine->getWindow().setSize(width(), height());
    m_engine->getScene().add(m_path.toStdString());
    auto& opt              = m_engine->getOptions();
    opt.render.grid.enable = true;
    // opt.ui.axis            = true;
    //  opt.render.background.color = {0, 0, 0};
}

void F3DWindow::resizeGL(int w, int h)
{
    if (m_engine) {
        m_engine->getWindow().setSize(w, h);
    }
}

void F3DWindow::paintGL()
{
    if (m_engine) {
        m_engine->getWindow().render();
    }
}

void F3DWindow::mousePressEvent(QMouseEvent* event)
{
    m_pos = event->pos();
}

void F3DWindow::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void F3DWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_engine) {
        return;
    }

    auto delta = event->position() - m_pos;
    m_pos      = event->position();
    auto& cam  = m_engine->getWindow().getCamera();

    if (event->buttons() & Qt::LeftButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            auto pos         = cam.getPosition();
            auto focal       = cam.getFocalPoint();
            double dist      = std::sqrt(std::pow(focal[0] - pos[0], 2)
                                         + std::pow(focal[1] - pos[1], 2)
                                         + std::pow(focal[2] - pos[2], 2));
            double pan_speed = dist * 0.001;
            cam.pan(-delta.x() * pan_speed, delta.y() * pan_speed);
        }
        else {
            cam.azimuth(delta.x() * g_rotate_speed);
            cam.elevation(delta.y() * g_rotate_speed);
        }
    }
    else if (event->buttons() & Qt::RightButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            try {
                auto& opt    = m_engine->getOptions();
                double angle = delta.x() * 0.5;
                double prevRotation
                    = std::get<double>(opt.get("hdri.rotation"));
                opt.set("hdri.rotation", prevRotation + angle);
            }
            catch (...) {
                qprintt << "Error rotating HDRI";
            }
        }
        else {
            cam.dolly(1.0 - delta.y() * g_zoom_speed);
        }
    }
}

void F3DWindow::wheelEvent(QWheelEvent* event)
{
    if (!m_engine) {
        return;
    }

    const float delta = event->angleDelta().y() * g_zoom_factor;
    auto& cam         = m_engine->getWindow().getCamera();
    if (event->modifiers() & Qt::ControlModifier) {
        cam.dolly(1.0 + delta);
    }
    else {
        cam.zoom(1.0 + delta);
    }
}

void F3DWindow::keyPressEvent(QKeyEvent* event)
{
    handleKey(event);
}

void F3DWindow::handleKey(QKeyEvent* event)
{
    if (!m_engine)
        return;

    auto& opt = m_engine->getOptions();
    // OPTIONS.md
    // TODO:
    // https://github.com/f3d-app/f3d/blob/362e0b5a24840c5ced151db19814d72108493f5d/doc/libf3d/OPTIONS.md
    // https://github.com/f3d-app/f3d/blob/362e0b5a24840c5ced151db19814d72108493f5d/library/src/interactor_impl.cxx
    // for (auto k : m_engine->getOptions().getAllNames()) {
    //     qprintt << k;
    // }

    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool ctrl  = event->modifiers() & Qt::ControlModifier;

    try {
        switch (event->key()) {
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6: {
            auto& cam  = m_engine->getWindow().getCamera();
            auto focal = cam.getFocalPoint();
            auto pos   = cam.getPosition();

            double dist = std::sqrt(std::pow(focal[0] - pos[0], 2)
                                    + std::pow(focal[1] - pos[1], 2)
                                    + std::pow(focal[2] - pos[2], 2));

            // [1~6] => [front, back, left, right, top, bottom]
            const QHash<int, QVector3D> directions{
                {Qt::Key_1, QVector3D(0, 0, -1)},
                {Qt::Key_2, QVector3D(0, 0, 1)},
                {Qt::Key_3, QVector3D(-1, 0, 0)},
                {Qt::Key_4, QVector3D(1, 0, 0)},
                {Qt::Key_5, QVector3D(0, 1, 0)},
                {Qt::Key_6, QVector3D(0, -1, 0)},
            };
            QVector3D up(0, 1, 0);
            const QVector3D direction = directions[event->key()];
            if (event->key() == Qt::Key_5) {
                up = QVector3D(0, 0, 1);
            }
            else if (event->key() == Qt::Key_6) {
                up = QVector3D(0, 0, -1);
            }
            QVector3D targetPos
                = QVector3D(focal[0], focal[1], focal[2]) - direction * dist;
            moveCameraTo(targetPos, QVector3D(focal[0], focal[1], focal[2]),
                         up);
            break;
        }
            ////////////////////////////////////////////////////////////////
            /// undone
        case Qt::Key_A: {
            if (shift) {
                opt.toggle("render.armature");
            }
            else {
                opt.toggle("render.antialiasing");
            }
            qprintt << "Antialiasing";
            break;
        }
        case Qt::Key_B: {
            opt.toggle("coloring.scalar_bar");
            break;
        }
        case Qt::Key_C: {
            if (ctrl) {
                QImage buf = grabFramebuffer();
                // grabFramebuffer().convertToFormat(
                // QImage::Format_RGB888);
                if (buf.isNull()) {
                    qprintt << "copy failed";
                }
                else {
                    qApp->clipboard()->setImage(buf);
                }
            }
            else {
                opt.toggle("coloring.array.location");
                qprintt << "coloring.array.location";
            }
            break;
        }
        case Qt::Key_S: {
            opt.toggle("coloring.array.name");
            qprintt << "coloring.array.name";
            break;
        }
        case Qt::Key_Y: {
            opt.toggle("coloring.array.component");
            qprintt << "coloring.array.component";
            break;
        }
        case Qt::Key_W: {
            opt.toggle("animation.index");
            qprintt << "animation.index";
            break;
        }
        case Qt::Key_V: {
            opt.toggle("render.volume");
            break;
        }
        case Qt::Key_I: {
            // opt.toggle("volume.invert");
            opt.toggle("ui.metadata");
            opt.toggle("ui.fps");
            // opt.toggle("ui.axis");
            // opt.ui.axis = !opt.ui.axis;
            break;
        }
        case Qt::Key_O: {
            opt.toggle("render.point_sprites");
            break;
        }
        case Qt::Key_P: {
            if (ctrl)
                opt.set("model.opacity",
                        std::get<double>(opt.get("model.opacity")) + 0.1);
            else if (shift)
                opt.set("model.opacity",
                        std::get<double>(opt.get("model.opacity")) - 0.1);
            else
                opt.toggle("render.translucency");
            break;
        }
        case Qt::Key_Q:
            opt.toggle("render.ambient_occlusion");
            break;
        case Qt::Key_T:
            opt.toggle("render.tone_mapping");
            break;
        case Qt::Key_E:
            opt.toggle("render.edges");
            break;
        case Qt::Key_X:
            opt.toggle("render.axes");
            break;
        case Qt::Key_G:
            // opt.toggle("render.grid.enable");
            opt.render.grid.enable = !opt.render.grid.enable;
            break;
        case Qt::Key_N:
            opt.toggle("render.filename");
            break;
        case Qt::Key_M:
            opt.toggle("render.metadata");
            break;
        case Qt::Key_Z:
            opt.toggle("render.fps");
            break;
        case Qt::Key_R:
            opt.toggle("render.raytracing");
            break;
        case Qt::Key_D:
            opt.toggle("render.denoiser");
            break;
        case Qt::Key_U:
            opt.toggle("render.background_blur");
            break;
        case Qt::Key_K:
            opt.toggle("interactor.trackball");
            break;
        case Qt::Key_F:
            opt.toggle("hdri.ambient");
            break;
        case Qt::Key_J:
            opt.toggle("hdri.skybox");
            break;
        case Qt::Key_L: {
            if (shift) {
                opt.set("light.intensity",
                        std::get<double>(opt.get("light.intensity")) - 0.1);
            }
            else {
                opt.set("light.intensity",
                        std::get<double>(opt.get("light.intensity")) + 0.1);
            }
            break;
        }
        case Qt::Key_Return:
            m_engine->getWindow().getCamera().resetToDefault();
            break;
        default:
            break;
        }
    }
    catch (...) {
        qprintt << "Error handling key" << event->key();
    }
}

void F3DWindow::moveCameraTo(const QVector3D& newPos,
                             const QVector3D& focal,
                             const QVector3D& up)
{
    if (!m_engine) {
        return;
    }

    auto animations = this->findChildren<QVariantAnimation*>();
    if (!animations.isEmpty()) {
        qprintt << "Animation already running";
        return;
    }

    const auto pos = m_engine->getWindow().getCamera().getPosition();
    m_animation.camera_pos_start = QVector3D(pos[0], pos[1], pos[2]);
    m_animation.camera_pos_end   = newPos;
    auto anim                    = new QVariantAnimation(this);
    anim->setDuration(500);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(anim, &QVariantAnimation::valueChanged, this,
            [this, focal, up](const QVariant& value) {
                double progress = value.toDouble();
                auto current = m_animation.camera_pos_start * (1.0 - progress)
                               + m_animation.camera_pos_end * progress;
                auto& cam = m_engine->getWindow().getCamera();
                cam.setPosition({current.x(), current.y(), current.z()});
                cam.setFocalPoint({focal.x(), focal.y(), focal.z()});
                cam.setViewUp({up.x(), up.y(), up.z()});
            });
    connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
    anim->start();
}
