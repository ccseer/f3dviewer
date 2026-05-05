#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include "F3DPathWorkaround.h"

class F3DPathWorkaroundTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void createsAliasUnderSeerAutodelWithoutHiddenAttribute();
};

void F3DPathWorkaroundTest::createsAliasUnderSeerAutodelWithoutHiddenAttribute()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temporary source directory should be created");

    const QString sourcePath = dir.filePath(QString::fromUtf8("模型123.fbx"));
    QFile source(sourcePath);
    QVERIFY2(source.open(QIODevice::WriteOnly),
             "source file should be writable");
    source.write("dummy");
    source.close();

    const QString aliasPath = f3d::workaround::createAsciiAlias(sourcePath);
    QVERIFY2(!aliasPath.isEmpty(), "alias path should be created");

    const QString expectedRoot
        = QDir::cleanPath(f3d::workaround::tempAliasRoot());
    QVERIFY(QDir::cleanPath(aliasPath).startsWith(expectedRoot + "/"));

    QVERIFY(QFileInfo(aliasPath).exists());
    QVERIFY(!QFileInfo(aliasPath).isHidden());
    QVERIFY(QFile::remove(aliasPath));
}

QTEST_APPLESS_MAIN(F3DPathWorkaroundTest)

#include "F3DPathWorkaround_test.moc"
