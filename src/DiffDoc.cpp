// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDoc.hpp"

namespace {

QString baseName(const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    return slash < 0 ? path : path.mid(slash + 1);
}

bool isRename(const FileDiff &file)
{
    if (file.oldPath.isEmpty() || file.newPath.isEmpty() || file.oldPath == file.newPath)
        return false;
    return file.oldPath != QLatin1String("/dev/null") && file.newPath != QLatin1String("/dev/null");
}

} // namespace

QString FileDiff::shortName() const
{
    if (!isRename(*this))
        return baseName(path());
    const QString left = baseName(oldPath);
    const QString right = baseName(newPath);
    if (left == right)
        return left;
    return left + QStringLiteral(" → ") + right;
}
