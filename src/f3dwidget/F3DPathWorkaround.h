#pragma once

#include <QString>

namespace f3d::workaround {

QString tempAliasRoot();
QString normalizeLoadPath(const QString &path);
QString createAsciiAlias(const QString &path);

}
