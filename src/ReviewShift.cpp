#include "ReviewShift.hpp"

#include "DiffDetail.hpp"

#include <QDir>
#include <QFile>

namespace {

bool sameFile(const FileDiff &file, const QString &path)
{
    return file.path() == path || file.oldPath == path || file.newPath == path;
}

void addMatch(QList<int> *lines, int line, const QString &text, const QString &snippet)
{
    if (line > 0 && text == snippet)
        lines->push_back(line);
}

QList<int> diffMatches(const DiffDoc &doc, const ReviewNote &note)
{
    QList<int> lines;
    for (const FileDiff &file : doc.files) {
        if (!sameFile(file, note.path))
            continue;
        for (const DiffRow &row : file.rows) {
            const int line = note.oldSide ? row.leftNum : row.rightNum;
            const QString text = note.oldSide ? row.leftText : row.rightText;
            addMatch(&lines, line, text, note.snippet);
        }
    }
    return lines;
}

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

QList<int> fileMatches(const QString &root, const DiffDoc &doc, const ReviewNote &note)
{
    QList<int> lines;
    if (root.isEmpty() || note.oldSide)
        return lines;
    QFile file(QDir(root).filePath(diskPath(doc, note.path)));
    if (!file.open(QIODevice::ReadOnly))
        return lines;
    int number = 0;
    while (!file.atEnd())
        addMatch(&lines, ++number, trimmedLine(&file), note.snippet);
    return lines;
}

int nearestShift(const QList<int> &lines, int anchor)
{
    int best = 0;
    int bestDist = -1;
    for (int line : lines) {
        if (line == anchor)
            return 0;
        const int dist = qAbs(line - anchor);
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            best = line;
        }
    }
    return best;
}

} // namespace

int shiftedLine(const DiffDoc &doc, const QString &root, const ReviewNote &note)
{
    if (note.snippet.isEmpty())
        return 0;
    return nearestShift(diffMatches(doc, note) + fileMatches(root, doc, note), note.line);
}
