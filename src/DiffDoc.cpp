// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDoc.hpp"

#include "DiffDetail.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLocale>

#include <algorithm>

namespace {

QString baseName(const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    return slash < 0 ? path : path.mid(slash + 1);
}

QStringList plainLines(const QByteArray &bytes)
{
    QStringList lines = QString::fromUtf8(bytes).split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    return lines;
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

QString FileDiff::modifiedText() const
{
    if (modified <= 0)
        return {};
    return QLocale::system().toString(QDateTime::fromMSecsSinceEpoch(modified), QLocale::ShortFormat);
}

QString FileDiff::markTip() const
{
    return opened ? QStringLiteral("Close") : QStringLiteral("Mark as Reviewed");
}

QString FileDiff::paneLabel(const QString &branch) const
{
    if (added)
        return branch + QStringLiteral(" (new file)");
    return branch;
}

void seedModified(FileDiff *file, const QString &root)
{
    if (!file || file->removed || root.isEmpty())
        return;
    const QFileInfo info(QDir(root).filePath(file->path()));
    if (info.exists())
        file->modified = info.lastModified().toMSecsSinceEpoch();
}

void seedModified(DiffDoc *doc, const QString &root)
{
    if (!doc || root.isEmpty())
        return;
    for (FileDiff &file : doc->files)
        seedModified(&file, root);
}

FileDiff unchangedFile(const QString &path, const QStringList &lines)
{
    FileDiff file;
    file.oldPath = path;
    file.newPath = path;
    for (int index = 0; index < lines.size(); ++index) {
        DiffRow row;
        row.kind = RowKind::Context;
        row.leftNum = index + 1;
        row.rightNum = index + 1;
        row.leftText = expandTabs(lines.at(index));
        row.rightText = row.leftText;
        file.rows.append(row);
    }
    return file;
}

bool loadPlain(const QString &root, const QString &path, FileDiff *file)
{
    if (!file || root.isEmpty() || path.isEmpty())
        return false;
    QFile input(QDir(root).filePath(path));
    if (!input.open(QIODevice::ReadOnly))
        return false;
    const QByteArray bytes = input.readAll();
    if (bytes.contains('\0'))
        return false;
    *file = unchangedFile(path, plainLines(bytes));
    file->opened = true;
    seedModified(file, root);
    return true;
}

int fileNamed(const DiffDoc &doc, const QString &path)
{
    for (int index = 0; index < doc.files.size(); ++index) {
        if (doc.files.at(index).path() == path)
            return index;
    }
    return -1;
}

int rowForLine(const FileDiff &file, int line, bool *oldSide)
{
    int left = -1;
    for (int index = 0; index < file.rows.size(); ++index) {
        const DiffRow &row = file.rows.at(index);
        if (row.rightNum == line) {
            *oldSide = false;
            return index;
        }
        if (left < 0 && row.leftNum == line)
            left = index;
    }
    *oldSide = left >= 0;
    return left;
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
