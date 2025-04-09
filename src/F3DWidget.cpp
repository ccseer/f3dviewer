#include "F3DWidget.h"

#include <f3d/engine.h>
#include <f3d/options.h>
#include <f3d/scene.h>
#include <f3d/window.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QQuaternion>
#include <QVariantAnimation>
#include <QVector3D>

#define qprintt qDebug() << "[F3DWindow]"

namespace {
constexpr auto g_shift_delta   = 0.1f;
constexpr float g_zoom_factor  = 0.001f;
constexpr float g_zoom_speed   = 0.01f;
constexpr float g_rotate_speed = 0.5f;
}  // namespace

F3DWidget::F3DWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    qprintt << this;
}

F3DWidget::~F3DWidget()
{
    qprintt << "~" << this;
    m_engine.reset();
    qprintt << "~" << this;
}

bool F3DWidget::load(const QString& path)
{
    try {
        connect(this, &QOpenGLWidget::frameSwapped, this, [this]() {
            //
            update();
        });
        QElapsedTimer et;
        et.start();
        f3d::engine::autoloadPlugins();
        m_engine = std::make_unique<f3d::engine>(
            f3d::engine::createExternal([this](const char* name) {
                return context()->getProcAddress(name);
            }));
        auto& opt              = m_engine->getOptions();
        opt.render.grid.enable = true;
        // opt.ui.loader_progress = true;
        // opt.render.background.color = {0, 0, 0};
        //  The default scene always has at most one animation.
        //  The animation index is 0 if no animation is present.
        opt.scene.animation.index = -1;

        m_engine->getWindow().setSize(width(), height());
        m_engine->getScene().add(path.toStdString());
        qprintt << "load" << et.elapsed();
        // initial state
        auto& cam = m_engine->getWindow().getCamera();
        cam.resetToBounds(0.7);
        cam.azimuth(45);
        cam.elevation(30);
        cam.setCurrentAsDefault();
        if (m_engine->getScene().animationTimeRange().second != 0.) {
            // 60 fps
            m_animation.timer.setInterval(16);
            connect(&m_animation.timer, &QTimer::timeout, this,
                    &F3DWidget::onAnimTick);
            m_animation.timer.start();
            m_animation.elapsed.start();
        }
        else {
            // no animation
            m_animation.playing = false;
        }
        return true;
    }
    catch (...) {
        qprintt << "Error initializing F3D engine";
        return false;
    }
}

void F3DWidget::initializeGL()
{
    QOpenGLWidget::initializeGL();
}

void F3DWidget::resizeGL(int w, int h)
{
    if (m_engine) {
        m_engine->getWindow().setSize(w, h);
    }
}

void F3DWidget::paintGL()
{
    if (m_engine) {
        m_engine->getWindow().render();
    }
}

void F3DWidget::mousePressEvent(QMouseEvent* event)
{
    m_pos = event->pos();
}

void F3DWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void F3DWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_engine) {
        return;
    }

    auto delta = event->position() - m_pos;
    m_pos      = event->position();
    auto& cam  = m_engine->getWindow().getCamera();

    auto panCamera = [this, &cam](float dx, float dy, double speedScale = 1.0) {
        auto pos         = cam.getPosition();
        auto focal       = cam.getFocalPoint();
        double dist      = std::sqrt(std::pow(focal[0] - pos[0], 2)
                                     + std::pow(focal[1] - pos[1], 2)
                                     + std::pow(focal[2] - pos[2], 2));
        double pan_speed = dist * 0.001 * speedScale;
        cam.pan(-dx * pan_speed, dy * pan_speed);
    };

    if (event->buttons() & Qt::LeftButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            panCamera(delta.x(), delta.y(), 1.);
        }
        else if (event->modifiers() & Qt::ControlModifier) {
            auto pos_arr = cam.getPosition();
            QVector3D pos(pos_arr[0], pos_arr[1], pos_arr[2]);
            auto focal_arr = cam.getFocalPoint();
            QVector3D focal(focal_arr[0], focal_arr[1], focal_arr[2]);
            auto rollRot = QQuaternion::fromAxisAndAngle(
                (focal - pos).normalized(), -delta.x() * g_rotate_speed);
            auto view_arr = cam.getViewUp();
            QVector3D up  = rollRot.rotatedVector(
                QVector3D(view_arr[0], view_arr[1], view_arr[2]));
            cam.setViewUp({up.x(), up.y(), up.z()});
        }
        else {
            cam.azimuth(-delta.x() * g_rotate_speed);
            cam.elevation(delta.y() * g_rotate_speed);
        }
    }
    else if (event->buttons() & Qt::RightButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            panCamera(delta.x(), delta.y(), g_shift_delta);
        }
        else {
            cam.dolly(1.0 - delta.y() * g_zoom_speed);
        }
    }
}

void F3DWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!m_engine || event->button() != Qt::LeftButton) {
        QOpenGLWidget::mouseDoubleClickEvent(event);
        return;
    }

    qprintt << "mouseDoubleClickEvent, resetting camera";
    m_engine->getWindow().getCamera().resetToDefault();
}

void F3DWidget::wheelEvent(QWheelEvent* event)
{
    if (!m_engine) {
        return;
    }

    const float delta = event->angleDelta().y() * g_zoom_factor;
    auto& cam         = m_engine->getWindow().getCamera();
    // if (event->modifiers() & Qt::ControlModifier) {
    //     cam.dolly(1.0 + delta);
    // }
    // else
    if (event->modifiers() & Qt::ShiftModifier) {
        cam.zoom(1.0 + delta * g_shift_delta);
    }
    else {
        cam.zoom(1.0 + delta);
    }
}

void F3DWidget::keyPressEvent(QKeyEvent* event)
{
    handleKey(event);
}

void F3DWidget::handleKey(QKeyEvent* event)
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
            // opt.toggle("scene.camera.orthographic");
            break;
        }
        case Qt::Key_8: {
            // isometric view?
            break;
        }
        // case Qt::Key_W: {
        //   not working
        //    opt.scene.animation.index += 1;
        //    qprintt << "animation index" << opt.scene.animation.index
        //            << m_engine->getScene().animationTimeRange();
        //    break;
        //}
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
                double opacity          = opt.model.color.opacity.value_or(0.0);
                opacity                 = std::min(opacity + 0.1, 1.0);
                opt.model.color.opacity = opacity;
            }
            else if (shift) {
                double opacity          = opt.model.color.opacity.value_or(1.0);
                opacity                 = std::max(opacity - 0.1, 0.1);
                opt.model.color.opacity = opacity;
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
            opt.toggle("model.volume.inverse");
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
        // case Qt::Key_R: {
        //     opt.toggle("render.raytracing.enable");
        //     break;
        // }
        // case Qt::Key_D: {
        //     opt.toggle("render.raytracing.denoise");
        //     break;
        // }
        default:
            break;
        }
    }
    catch (...) {
        qprintt << "Error handling key" << event->text();
    }
}

void F3DWidget::moveCameraTo(const QVector3D& new_pos,
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

    constexpr auto key_start = "camera_pos_start";
    constexpr auto key_end   = "camera_pos_end";
    const auto pos           = m_engine->getWindow().getCamera().getPosition();
    auto anim                = new QVariantAnimation(this);
    anim->setProperty(key_start, QVector3D(pos[0], pos[1], pos[2]));
    anim->setProperty(key_end, new_pos);
    anim->setDuration(500);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    connect(anim, &QVariantAnimation::valueChanged, this,
            [this, anim, focal, up](const QVariant& value) {
                const auto start = anim->property(key_start).value<QVector3D>();
                const auto end   = anim->property(key_end).value<QVector3D>();
                double progress  = value.toDouble();
                auto current     = start * (1.0 - progress) + end * progress;
                auto& cam        = m_engine->getWindow().getCamera();
                cam.setPosition({current.x(), current.y(), current.z()});
                cam.setFocalPoint({focal.x(), focal.y(), focal.z()});
                cam.setViewUp({up.x(), up.y(), up.z()});
            });
    connect(anim, &QVariantAnimation::finished, anim, &QObject::deleteLater);
    anim->start();
}

void F3DWidget::onAnimTick()
{
    if (!m_engine || !m_animation.playing) {
        return;
    }
    m_animation.pos
        += (m_animation.elapsed.restart() * 1. / 1000. * m_animation.speed);
    auto max = m_engine->getScene().animationTimeRange().second;
    if (max > 0.0) {
        m_animation.pos = std::fmod(m_animation.pos, max);
    }
    m_engine->getScene().loadAnimationTime(m_animation.pos);
}
