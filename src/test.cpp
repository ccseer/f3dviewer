#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QString>

#include "f3dviewer.h"

int main(int argc, char *argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    qunsetenv("QT_SCALE_FACTOR");
    QApplication app(argc, argv);
    QElapsedTimer et;
    et.start();

    ViewOptionsPrivate d;
    d.dpr         = 1;
    d.theme       = 1;
    d.path        = "C:\\Users\\corey\\Dev\\build_output\\f3dviewer\\src\\1.FBX";
    d.viewer_type = "f3dviewer";
    // d.path = "c:/d/1.gltf";
    // d.path = "c:/d/2.3ds";
    // d.path = "d:/3d/p.3ds";

    if (!QFile::exists(d.path)) {
        qDebug() << "file not found" << d.path;
        return -1;
    }

    ViewOptions opts;
    opts.d_ptr = &d;

    F3DViewer viewer;
    viewer.setWindowTitle(d.path);
    viewer.load(nullptr, &opts);
    qDebug() << "load" << et.restart() << "ms";
    viewer.resize(viewer.getContentSize());
    viewer.show();
    qDebug() << "show" << et.restart() << "ms";

    return app.exec();
}
