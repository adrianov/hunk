// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDoc.hpp"

#include <QHash>

#include <algorithm>

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

namespace {

struct FileGroup {
    QString folder;
    QList<int> files;
};

bool folderBefore(const FileGroup &left, const FileGroup &right)
{
    if (left.folder.isEmpty())
        return !right.folder.isEmpty();
    if (right.folder.isEmpty())
        return false;
    return left.folder.localeAwareCompare(right.folder) < 0;
}

void addListed(QList<FileGroup> *groups, QHash<QString, int> *index, int fileIndex, const QString &folder)
{
    if (!index->contains(folder)) {
        index->insert(folder, groups->size());
        groups->push_back(FileGroup{folder, {}});
    }
    (*groups)[index->value(folder)].files.push_back(fileIndex);
}

} // namespace

QList<int> listedOrder(const DiffDoc &doc)
{
    QList<FileGroup> groups;
    QHash<QString, int> index;
    for (int fileIndex = 0; fileIndex < doc.files.size(); ++fileIndex)
        addListed(&groups, &index, fileIndex, doc.files.at(fileIndex).folder());
    std::sort(groups.begin(), groups.end(), folderBefore);
    QList<int> order;
    for (const FileGroup &group : groups)
        order += group.files;
    return order;
}

QString FileDiff::title() const
{
    if (!isRename(*this))
        return path();
    const int slash = oldPath.lastIndexOf(QLatin1Char('/'));
    if (slash >= 0 && newPath.startsWith(oldPath.left(slash + 1)))
        return oldPath + QStringLiteral(" → ") + newPath.mid(slash + 1);
    return oldPath + QStringLiteral(" → ") + newPath;
}

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
