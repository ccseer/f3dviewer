#include "F3DPathWorkaround.h"

#include <windows.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace f3d::workaround {
namespace {

QString ensureTempAliasRoot()
{
    const QString root = QDir::cleanPath(
        QDir(QDir::tempPath()).filePath("Seer/autodel"));
    QDir().mkpath(root);
    return root;
}

QString aliasNameFor(const QFileInfo &info, int index)
{
    const QString ext    = info.completeSuffix().toLower();
    const QString dotExt = ext.isEmpty() ? QString() : "." + ext;
    return QString("seer_f3d_%1%2").arg(index, 2, 10, QChar('0')).arg(dotExt);
}

}

QString tempAliasRoot()
{
    return ensureTempAliasRoot();
}

QString normalizeLoadPath(const QString &path)
{
    const QFileInfo info(path);
    const QString absolute = info.absoluteFilePath();
    const QString native   = QDir::toNativeSeparators(absolute);
    std::wstring wide      = native.toStdWString();
    std::wstring shortPath(MAX_PATH, L'\0');
    const DWORD len = GetShortPathNameW(wide.c_str(), shortPath.data(),
                                        static_cast<DWORD>(shortPath.size()));
    if (len <= 0) {
        return absolute;
    }

    shortPath.resize(len);
    const QString shortened
        = QDir::fromNativeSeparators(QString::fromStdWString(shortPath));
    const QString originalSuffix = info.completeSuffix();
    const QString shortenedSuffix = QFileInfo(shortened).completeSuffix();
    if (!originalSuffix.compare(shortenedSuffix, Qt::CaseInsensitive)) {
        return shortened;
    }

    return absolute;
}

QString createAsciiAlias(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    const QString root = ensureTempAliasRoot();
    const QString src  = QDir::toNativeSeparators(info.absoluteFilePath());

    for (int i = 0; i < 64; ++i) {
        const QString aliasPath = QDir(root).filePath(aliasNameFor(info, i));
        if (QFileInfo::exists(aliasPath)) {
            continue;
        }

        const QString nativeAlias = QDir::toNativeSeparators(aliasPath);
        if (CreateHardLinkW(reinterpret_cast<LPCWSTR>(nativeAlias.utf16()),
                            reinterpret_cast<LPCWSTR>(src.utf16()),
                            nullptr)) {
            return aliasPath;
        }

        if (QFile::copy(info.absoluteFilePath(), aliasPath)) {
            return aliasPath;
        }
    }

    return {};
}

}
