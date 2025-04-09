#include <QApplication>
#include <QFileInfo>

#include "F3DWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString p = "C:/D/1.stl";
    p         = "C:/D/mr.fbx";
    // p         = "C:/D/a.glb";
    ///
    // p         = "D:/3d/1.stl";
    // p         = "D:/3d/cube.fbx";
    // p         = "D:/3d/c.stl";
    // p = "D:/3d/mr.fbx";
    // p = "D:/3d/a.glb";
    // p = "D:/3d/p.3ds";
    if (!QFileInfo::exists(p)) {
        qDebug() << "File does not exist:" << p;
        return -1;
    }
    F3DWidget *viewer = new F3DWidget();
    if (!viewer->load(p)) {
        return -1;
    }
    viewer->resize(1280, 720);
    viewer->show();

    return app.exec();
}
