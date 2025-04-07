#include <QApplication>
#include <QFileInfo>

#include "F3DWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString p = "C:/D/1.gltf";
    p         = "D:/3d/1.stl";
    //p         = "D:/3d/cube.fbx";
    //p         = "D:/3d/c.stl";
     p = "D:/3d/mr.fbx";
     //p = "D:/3d/p.3ds";
    if (!QFileInfo::exists(p)) {
        qDebug() << "File does not exist:" << p;
        return -1;
    }
    F3DWindow *viewer = new F3DWindow();
    if (!viewer->load(p)) {
        return -1;
    }
    viewer->resize(1280, 720);
    viewer->show();

    return app.exec();
}
