// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "ReviewShift.hpp"

#include "DiffDetail.hpp"
#include "ReviewMatch.hpp"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QSet>

namespace {

QString diskPath(const DiffDoc &doc, const QString &path)
{
    for (const FileDiff &file : doc.files) {
        if (sameFile(file, path))
            return file.path();
    }
    return path;
}

QString trimmedLine(QFile *file)
{
    QString text = QString::fromUtf8(file->readLine());
    if (text.endsWith(QLatin1Char('\n')))
        text.chop(1);
    if (text.endsWith(QLatin1Char('\r')))
        text.chop(1);
    return expandTabs(text);
}

QStringList readLines(const QString &root, const QString &path)
{
    QStringList lines;
    QFile file(QDir(root).filePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return lines;
    while (!file.atEnd())
        lines.push_back(trimmedLine(&file));
    return lines;
}

QHash<int, QString> sideMap(const DiffDoc &doc, const ReviewNote &note)
{
    QHash<int, QString> lines;
    for (const FileDiff &file : doc.files) {
        if (!sameFile(file, note.path))
            continue;
        for (const DiffRow &row : file.rows) {
            const int number = note.oldSide ? row.leftNum : row.rightNum;
            if (number > 0)
                lines.insert(number, sideText(row, note.oldSide));
        }
    }
    return lines;
}

bool sameRun(const QStringList &parts, const QStringList &lines, int start)
{
    if (start <= 0 || start + parts.size() - 1 > lines.size())
        return false;
    for (int index = 0; index < parts.size(); ++index) {
        if (lines.at(start - 1 + index) != parts.at(index))
            return false;
    }
    return !parts.isEmpty();
}

bool sameDiffRun(const QStringList &parts, const QHash<int, QString> &lines, int start)
{
    for (int index = 0; index < parts.size(); ++index) {
        const int line = start + index;
        if (!lines.contains(line) || lines.value(line) != parts.at(index))
            return false;
    }
    return !parts.isEmpty();
}

void collectSpan(QSet<int> *starts, const QStringList &parts, const QHash<int, QString> &diffLines,
                  const QStringList &fileLines, int anchor)
{
    for (int line : diffLines.keys()) {
        if (sameDiffRun(parts, diffLines, line))
            starts->insert(line);
    }
    QSet<int> fileHits;
    bool atAnchor = false;
    for (int index = 0; index + parts.size() <= fileLines.size(); ++index) {
        const int line = index + 1;
        if (!sameRun(parts, fileLines, line))
            continue;
        if (line == anchor)
            atAnchor = true;
        else
            fileHits.insert(line);
    }
    if (!atAnchor)
        starts->unite(fileHits);
    // No diff copy: the unchanged worktree lines still hold the comment.
    else if (starts->isEmpty())
        starts->insert(anchor);
}

} // namespace

QStringList cachedFile(const DiffDoc &doc, const QString &root, const QString &path, LineCache *cache)
{
    const QString key = diskPath(doc, path);
    if (!cache->files.contains(key))
        cache->files.insert(key, readLines(root, key));
    return cache->files.value(key);
}

int spanShift(const DiffDoc &doc, const QString &root, const ReviewNote &note, LineCache *cache)
{
    if (note.snippet.isEmpty() || note.end <= note.line)
        return 0;
    const QStringList parts = note.snippet.split(QLatin1Char('\n'));
    QSet<int> starts;
    QStringList fileLines;
    if (!root.isEmpty() && !note.oldSide)
        fileLines = cachedFile(doc, root, note.path, cache);
    collectSpan(&starts, parts, sideMap(doc, note), fileLines, note.line);
    if (starts.size() != 1)
        return 0;
    return *starts.constBegin();
}
