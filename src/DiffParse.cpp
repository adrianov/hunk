#include "DiffParse.hpp"

#include "DiffDetail.hpp"

#include <QRegularExpression>

namespace {

void countChanges(FileDiff &file)
{
    for (const DiffRow &row : file.rows) {
        if (row.kind == RowKind::Add || row.kind == RowKind::Mod)
            ++file.adds;
        if (row.kind == RowKind::Del || row.kind == RowKind::Mod)
            ++file.dels;
    }
}

void assignPaths(ParseCursor &cur, const QString &line)
{
    QString oldPath;
    QString newPath;
    readPath(line, readPath(line, 11, &oldPath), &newPath);
    cur.file.oldPath = oldPath;
    cur.file.newPath = newPath;
}

void pushRow(ParseCursor &cur, QChar kind, const QString &text)
{
    RawLine rec;
    rec.kind = kind;
    rec.text = text;
    if (kind != QLatin1Char('+')) {
        if (cur.left > 0)
            rec.left = cur.left;
        ++cur.left;
    }
    if (kind != QLatin1Char('-')) {
        if (cur.right > 0)
            rec.right = cur.right;
        ++cur.right;
    }
    cur.lines.push_back(rec);
}

bool samePath(const FileDiff &file, const QString &path)
{
    return file.path() == path || file.oldPath == path || file.newPath == path;
}

void feedLine(ParseCursor &cur, DiffDoc &doc, const QString &line)
{
    if (startFile(cur, doc, line))
        return;
    if (cur.file.oldPath.isEmpty() && cur.file.newPath.isEmpty())
        return;
    if (takeHeader(cur, line) || cur.skipping)
        return;
    if (takeHunk(cur, line))
        return;
    takeRow(cur, line);
}

} // namespace

void finishFile(ParseCursor &cur, DiffDoc &doc)
{
    if (cur.file.oldPath.isEmpty() && cur.file.newPath.isEmpty())
        return;
    if (cur.file.oldPath == QLatin1String("/dev/null"))
        cur.file.added = true;
    if (cur.file.newPath == QLatin1String("/dev/null"))
        cur.file.removed = true;
    if (!cur.file.binary)
        cur.file.rows = zipRows(cur.lines);
    countChanges(cur.file);
    doc.files.push_back(cur.file);
}

bool startFile(ParseCursor &cur, DiffDoc &doc, const QString &line)
{
    if (!line.startsWith(QLatin1String("diff --git ")))
        return false;
    finishFile(cur, doc);
    cur = ParseCursor();
    assignPaths(cur, line);
    return true;
}

bool takeHunk(ParseCursor &cur, const QString &line)
{
    if (!line.startsWith(QLatin1String("@@")))
        return false;
    static const QRegularExpression hunkRe(QStringLiteral("^@@ -(\\d+)(?:,\\d+)? \\+(\\d+)(?:,\\d+)? @@"));
    const auto match = hunkRe.match(line);
    if (match.hasMatch()) {
        cur.left = match.captured(1).toInt();
        cur.right = match.captured(2).toInt();
        cur.inHunk = true;
    }
    return true;
}

void takeRow(ParseCursor &cur, const QString &line)
{
    if (!cur.inHunk || line.isEmpty() || line.startsWith(QLatin1Char('\\')))
        return;
    const QChar kind = line.at(0);
    if (kind != QLatin1Char(' ') && kind != QLatin1Char('+') && kind != QLatin1Char('-'))
        return;
    pushRow(cur, kind, expandTabs(line.mid(1)));
}

DiffDoc parseDiff(const QString &raw)
{
    DiffDoc doc;
    ParseCursor cur;
    for (QString line : raw.split(QLatin1Char('\n'))) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        feedLine(cur, doc, line);
    }
    finishFile(cur, doc);
    return doc;
}

LineHit findLine(const DiffDoc &doc, const QString &path, bool oldSide, int line)
{
    for (int fileIndex = 0; fileIndex < doc.files.size(); ++fileIndex) {
        const FileDiff &file = doc.files.at(fileIndex);
        if (!samePath(file, path))
            continue;
        for (int rowIndex = 0; rowIndex < file.rows.size(); ++rowIndex) {
            const int number = oldSide ? file.rows.at(rowIndex).leftNum : file.rows.at(rowIndex).rightNum;
            if (number == line)
                return LineHit{fileIndex, rowIndex};
        }
    }
    return {};
}
