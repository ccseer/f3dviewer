#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QString>

#include "f3dviewer.h"

int main(int argc, char* argv[])
{
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    qunsetenv("QT_SCALE_FACTOR");
    QApplication app(argc, argv);
    QElapsedTimer et;
    et.start();
    F3DViewer viewer;

    auto p     = std::make_unique<ViewOptions>();
    p->d->dpr  = 1;
    p->d->path = "c:/d/1.gltf";
    p->d->path = "c:/d/2.3ds";
    if (!QFile::exists(p->d->path)) {
        qDebug() << "file not found" << p->d->path;
        return -1;
    }
    p->d->theme = 1;
    p->d->type  = viewer.name();
    viewer.setWindowTitle(p->d->path);
    viewer.load(nullptr, std::move(p));
    qDebug() << "load" << et.restart() << "ms";
    viewer.resize(viewer.getContentSize());
    viewer.show();
    qDebug() << "show" << et.restart() << "ms";

    return app.exec();
}
