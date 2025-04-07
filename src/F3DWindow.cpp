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

    f3d::context::function ctx = [this](const char* name) {
        return this->context()->getProcAddress(name);
    };
    try {
        f3d::engine::autoloadPlugins();
        m_engine
            = std::make_unique<f3d::engine>(f3d::engine::createExternal(ctx));
        m_engine->getWindow().setSize(width(), height());
        m_engine->getScene().add(m_path.toStdString());
        auto& opt              = m_engine->getOptions();
        opt.render.grid.enable = true;
        // initial state
        auto& cam = m_engine->getWindow().getCamera();
        cam.resetToBounds(0.7);
        cam.azimuth(45);
        cam.elevation(30);
        cam.setCurrentAsDefault();
    }
    catch (...) {
        qprintt << "Error initializing F3D engine";
        return;
    }
    // opt.render.show_edges        = true;
    // opt.scene.animation.autoplay = true;
    //  opt.ui.axis            = true;
    //   opt.render.background.color = {0, 0, 0};
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
            cam.azimuth(-delta.x() * g_rotate_speed);
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

void F3DWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_engine || event->button() != Qt::LeftButton) {
        QOpenGLWindow::mouseDoubleClickEvent(event);
        return;
    }

    qprintt << "mouseDoubleClickEvent, resetting camera";
    m_engine->getWindow().getCamera().resetToDefault();
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
    if (!m_engine) {
        return;
    }

    auto& opt        = m_engine->getOptions();
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool ctrl  = event->modifiers() & Qt::ControlModifier;

    // https://github.com/f3d-app/f3d/blob/master/doc/libf3d/OPTIONS.md
    try {
        switch (event->key()) {
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6: {
            auto& cam        = m_engine->getWindow().getCamera();
            const auto focal = cam.getFocalPoint();
            const auto pos   = cam.getPosition();
            double dist      = std::sqrt(std::pow(focal[0] - pos[0], 2)
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
            auto target
                = QVector3D(focal[0], focal[1], focal[2]) - direction * dist;
            moveCameraTo(target, QVector3D(focal[0], focal[1], focal[2]), up);
            break;
        }
        case Qt::Key_7: {
            opt.toggle("scene.camera.orthographic");
            break;
        }
        case Qt::Key_8: {
            // isometric view?
            break;
        }
        case Qt::Key_W: {
            // TODO:
            opt.toggle("animation.index");
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
            // else {
            //     opt.toggle("coloring.array.location");
            //     qprintt << "coloring.array.location";
            // }
            break;
        }
        // case Qt::Key_S: {
        //     break;
        // }
        // case Qt::Key_Y: {
        //      break;
        //  }
        case Qt::Key_B: {
            opt.toggle("ui.scalar_bar");
            break;
        }
        case Qt::Key_P: {
            if (ctrl) {
                if (opt.model.color.opacity.has_value()) {
                    if (opt.model.color.opacity.value() < 1.0) {
                        opt.model.color.opacity.value() += 0.1;
                    }
                }
                else {
                    opt.set("model.color.opacity", 0.1);
                }
            }
            else if (shift) {
                if (opt.model.color.opacity.has_value()) {
                    if (opt.model.color.opacity.value() > 0.1) {
                        opt.model.color.opacity.value() -= 0.1;
                    }
                }
                else {
                    opt.set("model.color.opacity", 0.9);
                }
            }
            else {
                opt.toggle("render.effect.translucency_support");
            }
            break;
        }
        case Qt::Key_Q: {
            opt.toggle("render.effect.ambient_occlusion");
            break;
        }
        case Qt::Key_A: {
            if (shift) {
                opt.toggle("render.armature.enable");
            }
            else {
                opt.toggle("render.effect.anti_aliasing");
            }
            break;
        }
        case Qt::Key_T: {
            opt.toggle("render.effect.tone_mapping");
            break;
        }
        case Qt::Key_E: {
            opt.toggle("render.show_edges");
            break;
        }
            // not supported by f3d, requires interactor
            // case Qt::Key_X:
            //     opt.toggle("render.axes");
            //     break;
        case Qt::Key_G: {
            opt.toggle("render.grid.enable");
            break;
        }
            // no need
            // case Qt::Key_N: {
            //     opt.toggle("render.filename");
            //     break;
            // }
        case Qt::Key_M: {
            opt.toggle("ui.metadata");
            break;
        }
        case Qt::Key_Z: {
            opt.toggle("ui.fps");
            break;
        }
        case Qt::Key_V: {
            opt.toggle("model.volume.enable");
            break;
        }
        case Qt::Key_I: {
            opt.toggle("opt.model.volume.inverse");
            break;
        }
        case Qt::Key_O: {
            opt.toggle("model.point_sprites.enable");
            break;
        }
        case Qt::Key_U: {
            opt.toggle("render.background.blur.enable");
            break;
        }
            // case Qt::Key_K: {
            //     opt.toggle("interactor.trackball");
            //     break;
            // }
        case Qt::Key_F: {
            opt.toggle("render.hdri.ambient");
            break;
        }
        case Qt::Key_J: {
            opt.toggle("render.background.skybox");
            break;
        }
        case Qt::Key_L: {
            if (shift) {
                opt.render.light.intensity -= 0.1;
            }
            else {
                opt.render.light.intensity += 0.1;
            }
            break;
        }
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            m_engine->getWindow().getCamera().resetToDefault();
            break;
        }
            ////////////////////////////////////////////////////////////////
            // TODO:
        // case Qt::Key_R:
        //     opt.toggle("render.raytracing");
        //     break;
        // case Qt::Key_D:
        //     opt.toggle("render.denoiser");
        //     break;
        default:
            break;
        }
    }
    catch (...) {
        qprintt << "Error handling key" << event->text();
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
        qprintt << "animation already running";
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
