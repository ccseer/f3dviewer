#include "F3DWidget.h"

#include <f3d/engine.h>

#include <filesystem>
#if __has_include(<f3d/log.h>)
#include <f3d/log.h>
#define F3DVIEWER_HAS_F3D_LOG 1
#endif
#include <f3d/options.h>
#include <f3d/scene.h>
#include <f3d/window.h>
#include <windows.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QQuaternion>
#include <QVariantAnimation>
#include <QVector3D>

#define qprintt qDebug() << "[F3DViewer]"

namespace {
constexpr auto g_shift_delta   = 0.1f;
constexpr float g_zoom_factor  = 0.001f;
constexpr float g_rotate_speed = 0.5f;

#ifdef F3DVIEWER_HAS_F3D_LOG
void initF3DLogging()
{
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    f3d::log::setUseColoring(false);
    f3d::log::forward(
        [](f3d::log::VerboseLevel level, const std::string &message) {
            const QString msg = QString::fromStdString(message).trimmed();
            if (msg.isEmpty()) {
                return;
            }
            switch (level) {
            case f3d::log::VerboseLevel::DEBUG:
                // qDebug() << "[F3D] [debug]" << msg;
                break;
            case f3d::log::VerboseLevel::INFO:
                // qInfo() << "[F3D] [info]" << msg;
                break;
            case f3d::log::VerboseLevel::WARN:
                qprintt << "[F3D] [warn]" << msg;
                break;
            case f3d::log::VerboseLevel::QUIET:
                break;
            default:
                // case f3d::log::VerboseLevel::ERROR: //compile error
                qprintt << "[F3D] [error]" << msg;
            }
        });
    f3d::log::setVerboseLevel(f3d::log::VerboseLevel::QUIET, true);
}
#endif

std::filesystem::path toFsPath(const QString &path)
{
    return std::filesystem::path(path.toStdWString());
}

QString normalizeLoadPath(const QString &path)
{
    const QString native
        = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
    std::wstring wide = native.toStdWString();
    std::wstring shortPath(MAX_PATH, L'\0');
    const DWORD len = GetShortPathNameW(wide.c_str(), shortPath.data(),
                                        static_cast<DWORD>(shortPath.size()));
    if (len > 0) {
        shortPath.resize(len);
        return QDir::fromNativeSeparators(QString::fromStdWString(shortPath));
    }
    return QFileInfo(path).absoluteFilePath();
}

bool hasNonAscii(const QString &path)
{
    for (const QChar ch : path) {
        if (ch.unicode() > 127) {
            return true;
        }
    }
    return false;
}

QString createAsciiSiblingAlias(const QString &path)
{
    QFileInfo info(path);
    const QString dir    = info.absolutePath();
    const QString ext    = info.completeSuffix();
    const QString dotExt = ext.isEmpty() ? QString() : "." + ext;
    const QString src    = QDir::toNativeSeparators(info.absoluteFilePath());

    for (int i = 0; i < 64; ++i) {
        const QString aliasName
            = QString("seer_f3d_%1%2").arg(i, 2, 10, QChar('0')).arg(dotExt);
        const QString aliasPath = QDir(dir).filePath(aliasName);
        if (QFileInfo::exists(aliasPath)) {
            continue;
        }
        if (CreateHardLinkW(reinterpret_cast<LPCWSTR>(
                                QDir::toNativeSeparators(aliasPath).utf16()),
                            reinterpret_cast<LPCWSTR>(src.utf16()), nullptr)) {
            return aliasPath;
        }
    }
    return {};
}
}  // namespace

F3DWidget::F3DWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    qprintt << this;
    setFocusPolicy(Qt::StrongFocus);
#ifdef F3DVIEWER_HAS_F3D_LOG
    initF3DLogging();
#endif
    qprintt << "f3d version" << f3d::engine::getLibInfo().VersionFull;
}

F3DWidget::~F3DWidget()
{
    makeCurrent();
    m_engine.reset();
    doneCurrent();
    if (!m_load_alias_path.isEmpty()) {
        QFile::remove(m_load_alias_path);
    }
    qprintt << "~" << this;
}

bool F3DWidget::load(const QString &path)
{
    m_original_path = QFileInfo(path).absoluteFilePath();
    m_path          = normalizeLoadPath(m_original_path);
    return true;
}

void F3DWidget::initializeGL()
{
    QOpenGLWidget::initializeGL();

    if (m_path.isEmpty()) {
        return;
    }

    try {
        qprintt << "f3d version" << f3d::engine::getLibInfo().VersionFull;
        f3d::engine::autoloadPlugins();
        m_engine = std::make_unique<f3d::engine>(
            f3d::engine::createExternal([this](const char *name) {
                return context()->getProcAddress(name);
            }));
        auto &opt     = m_engine->getOptions();
        auto setUiOpt = [&opt](const char *key, const char *value) {
            try {
                opt.setAsString(key, value);
            }
            catch (...) {
            }
        };
        opt.render.grid.enable = true;
        opt.ui.axis            = true;
        setUiOpt("scene.animation.indices", "-1");
        setUiOpt("ui.drop_zone.enable", "0");
        setUiOpt("ui.drop_zone.show_logo", "0");
        setUiOpt("ui.notifications.enable", "0");
        setUiOpt("ui.notifications.show_bindings", "0");
        setUiOpt("ui.cheatsheet", "0");
        setUiOpt("ui.console", "0");
        setUiOpt("ui.minimal_console", "0");
        setUiOpt("ui.filename", "0");
        setUiOpt("ui.animation_progress", "0");
        setUiOpt("ui.loader_progress", "0");

        m_engine->getWindow().setSize(width(), height());

        // Load model in background thread
        loadModelInBackground();

        connect(this, &QOpenGLWidget::frameSwapped, this,
                [this]() { update(); });
    }
    catch (const std::exception &e) {
        qprintt << "Error initializing F3D engine:" << e.what();
    }
    catch (...) {
        qprintt << "Error initializing F3D engine";
    }
}

void F3DWidget::loadModelInBackground()
{
    if (m_loading) {
        return;
    }
    m_loading = true;

    QTimer::singleShot(0, this, [this]() {
        try {
            m_engine->getScene().add(toFsPath(m_path));

            // initial state
            auto &cam = m_engine->getWindow().getCamera();
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

            m_loading = false;
            emit sigLoaded();
            emit sigAnimationStateChanged(m_animation.playing);
            emit sigAnimationProgressChanged(m_animation.pos,
                                             getAnimationDuration());
            update();
        }
        catch (const std::exception &e) {
            qprintt << "Error loading model:" << e.what() << "path:" << m_path;
            if (m_load_alias_path.isEmpty() && hasNonAscii(m_original_path)) {
                m_load_alias_path = createAsciiSiblingAlias(m_original_path);
                if (!m_load_alias_path.isEmpty()) {
                    try {
                        m_path = normalizeLoadPath(m_load_alias_path);
                        qprintt << "Retry loading via alias:" << m_path;
                        m_engine->getScene().add(toFsPath(m_path));

                        auto &cam = m_engine->getWindow().getCamera();
                        cam.resetToBounds(0.7);
                        cam.azimuth(45);
                        cam.elevation(30);
                        cam.setCurrentAsDefault();

                        if (m_engine->getScene().animationTimeRange().second
                            != 0.) {
                            m_animation.timer.setInterval(16);
                            connect(&m_animation.timer, &QTimer::timeout, this,
                                    &F3DWidget::onAnimTick);
                            m_animation.timer.start();
                            m_animation.elapsed.start();
                        }
                        else {
                            m_animation.playing = false;
                        }

                        m_loading = false;
                        emit sigLoaded();
                        emit sigAnimationStateChanged(m_animation.playing);
                        emit sigAnimationProgressChanged(
                            m_animation.pos, getAnimationDuration());
                        update();
                        return;
                    }
                    catch (const std::exception &retry) {
                        qprintt << "Retry loading model failed:" << retry.what()
                                << "alias:" << m_path;
                    }
                    catch (...) {
                        qprintt << "Retry loading model failed"
                                << "alias:" << m_path;
                    }
                }
            }
            m_loading = false;
        }
        catch (...) {
            qprintt << "Error loading model";
            m_loading = false;
        }
    });
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

void F3DWidget::mousePressEvent(QMouseEvent *event)
{
    m_pos = event->pos();
}

void F3DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

void F3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_engine) {
        return;
    }

    auto delta = event->position() - m_pos;
    m_pos      = event->position();
    auto &cam  = m_engine->getWindow().getCamera();

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
        if (event->modifiers() == Qt::NoModifier) {
            cam.azimuth(-delta.x() * g_rotate_speed);
            cam.elevation(delta.y() * g_rotate_speed);
        }
        else if (event->modifiers() & Qt::ShiftModifier) {
            cam.azimuth(-delta.x() * g_rotate_speed * g_shift_delta);
            cam.elevation(delta.y() * g_rotate_speed * g_shift_delta);
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
    }
    else if (event->buttons() & Qt::RightButton) {
        panCamera(delta.x(), delta.y(),
                  event->modifiers() & Qt::ShiftModifier ? g_shift_delta : 1.);
    }
}

void F3DWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_engine || event->button() != Qt::LeftButton) {
        QOpenGLWidget::mouseDoubleClickEvent(event);
        return;
    }

    qprintt << "mouseDoubleClickEvent, resetting camera";
    m_engine->getWindow().getCamera().resetToDefault();
}

void F3DWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_engine) {
        return;
    }

    const float delta = event->angleDelta().y() * g_zoom_factor;
    m_engine->getWindow().getCamera().zoom(
        1.0
        + (event->modifiers() & Qt::ShiftModifier ? delta * g_shift_delta
                                                  : delta));
}

void F3DWidget::keyPressEvent(QKeyEvent *event)
{
    handleKey(event);
}

void F3DWidget::handleKey(QKeyEvent *event)
{
    if (!m_engine) {
        return;
    }

    auto &opt        = m_engine->getOptions();
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
            moveCamera((CameraPos)event->key());
            break;
        }
        // case Qt::Key_7: {
        //     // opt.toggle("scene.camera.orthographic");
        //     break;
        // }
        // case Qt::Key_8: {
        //     // isometric view?
        //     break;
        // }
        //  case Qt::Key_W: {
        //    not working
        //     opt.scene.animation.index += 1;
        //     qprintt << "animation index" << opt.scene.animation.index
        //             << m_engine->getScene().animationTimeRange();
        //     break;
        // }
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
                opacity                 = qMin(opacity + 0.1, 1.0);
                opt.model.color.opacity = opacity;
            }
            else if (shift) {
                double opacity          = opt.model.color.opacity.value_or(1.0);
                opacity                 = qMax(opacity - 0.1, 0.1);
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
            moveCamera(CP_Default);
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

void F3DWidget::moveCamera(CameraPos cp)
{
    if (cp == CP_Default) {
        m_engine->getWindow().getCamera().resetToDefault();
        return;
    }
    auto &cam        = m_engine->getWindow().getCamera();
    const auto focal = cam.getFocalPoint();
    const auto pos   = cam.getPosition();
    double dist      = std::sqrt(std::pow(focal[0] - pos[0], 2)
                                 + std::pow(focal[1] - pos[1], 2)
                                 + std::pow(focal[2] - pos[2], 2));
    const QHash<int, QVector3D> directions{
        {CP_Front, QVector3D(0, 0, -1)}, {CP_Back, QVector3D(0, 0, 1)},
        {CP_Left, QVector3D(-1, 0, 0)},  {CP_Right, QVector3D(1, 0, 0)},
        {CP_Top, QVector3D(0, 1, 0)},    {CP_Bottom, QVector3D(0, -1, 0)},
    };
    QVector3D up(0, 1, 0);
    const QVector3D direction = directions[cp];
    if (cp == CP_Top) {
        up = QVector3D(0, 0, 1);
    }
    else if (cp == CP_Bottom) {
        up = QVector3D(0, 0, -1);
    }
    auto target = QVector3D(focal[0], focal[1], focal[2]) - direction * dist;
    moveCameraTo(target, QVector3D(focal[0], focal[1], focal[2]), up);
}

void F3DWidget::moveCameraTo(const QVector3D &new_pos,
                             const QVector3D &focal,
                             const QVector3D &up)
{
    if (!m_engine) {
        return;
    }
    auto animations = this->findChildren<QVariantAnimation *>();
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
            [this, anim, focal, up](const QVariant &value) {
                const auto start = anim->property(key_start).value<QVector3D>();
                const auto end   = anim->property(key_end).value<QVector3D>();
                double progress  = value.toDouble();
                auto current     = start * (1.0 - progress) + end * progress;
                auto &cam        = m_engine->getWindow().getCamera();
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
    emit sigAnimationProgressChanged(m_animation.pos, max);
}

void F3DWidget::setOption(const QString &key, const QString &v)
{
    if (!m_engine) {
        return;
    }
    try {
        m_engine->getOptions().setAsString(key.toStdString(), v.toStdString());
    }
    catch (...) {
        qprintt << "Error setting option" << key << v;
    }
}

QVariant F3DWidget::getOption(const QString &key) const
{
    if (!m_engine) {
        return {};
    }
    QVariant ret;
    try {
        ret = QVariant::fromStdVariant(
            m_engine->getOptions().get(key.toStdString()));
    }
    catch (...) {
        qprintt << "Error getting option" << key;
    }

    return ret;
}

bool F3DWidget::isAxisVisible() const
{
    return getOption("ui.axis").toBool();
}

bool F3DWidget::hasAnimation() const
{
    return m_engine && m_engine->getScene().animationTimeRange().second > 0.;
}

void F3DWidget::setAnimationState(bool play)
{
    if (!hasAnimation()) {
        return;
    }
    m_animation.playing = play;
    if (play) {
        m_animation.elapsed.restart();
        m_animation.timer.start();
    }
    else {
        m_animation.timer.stop();
    }
    emit sigAnimationStateChanged(m_animation.playing);
}

bool F3DWidget::isAnimationRunning() const
{
    if (!hasAnimation()) {
        return false;
    }
    return m_animation.playing;
}

void F3DWidget::setAnimationSpeed(double speed)
{
    m_animation.speed = speed;
}

double F3DWidget::getAnimationSpeed() const
{
    return m_animation.speed;
}

double F3DWidget::getAnimationPosition() const
{
    return m_animation.pos;
}

double F3DWidget::getAnimationDuration() const
{
    if (!m_engine) {
        return 0.0;
    }
    return m_engine->getScene().animationTimeRange().second;
}

void F3DWidget::setUIScale(double scale)
{
    if (!m_engine) {
        return;
    }
    try {
        m_engine->getOptions().setAsString("ui.scale", std::to_string(scale));
    }
    catch (...) {
        qprintt << "Error setting ui.scale" << scale;
    }
}

void F3DWidget::applyOptions(const QStringList &args)
{
    if (!m_engine) {
        return;
    }
    // Seer splits args by space into individual tokens: ["--key", "value"]
    // Also handle single-string form: ["--key value"]
    for (int i = 0; i < args.size(); ++i) {
        QString key, value;
        const QString &tok = args[i];
        if (tok.startsWith("--")) {
            if (tok.contains('=')) {
                int idx = tok.indexOf('=');
                key     = tok.mid(2, idx - 2);
                value   = tok.mid(idx + 1);
            }
            else if (tok.contains(' ')) {
                auto parts = tok.split(' ', Qt::SkipEmptyParts);
                if (parts.size() == 2) {
                    key   = parts[0].mid(2);
                    value = parts[1];
                }
            }
            else if (i + 1 < args.size() && !args[i + 1].startsWith("--")) {
                key   = tok.mid(2);
                value = args[++i];
            }
        }
        if (key.isEmpty() || value.isEmpty()) {
            continue;
        }
        try {
            // normalize "0"/"1" to "false"/"true" for boolean options
            QString v = value;
            if (v == "1")
                v = "true";
            else if (v == "0")
                v = "false";
            m_engine->getOptions().setAsString(key.toStdString(),
                                               v.toStdString());
            qprintt << "applyOptions:" << key << "=" << v;
        }
        catch (...) {
            qprintt << "Error applying option" << key << value;
        }
    }
}
