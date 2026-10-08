// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDetail.hpp"

namespace {

int readPlain(const QString &text, int index, QString *path)
{
    int end = index;
    while (end < text.size() && !text.at(end).isSpace())
        ++end;
    *path = text.mid(index, end - index);
    return end;
}

QChar unescape(QChar next)
{
    if (next == QLatin1Char('n'))
        return QLatin1Char('\n');
    if (next == QLatin1Char('t'))
        return QLatin1Char('\t');
    return next;
}

void appendQuoted(const QString &text, int *index, QString *value)
{
    if (text.at(*index) == QLatin1Char('\\') && *index + 1 < text.size()) {
        value->append(unescape(text.at(*index + 1)));
        *index += 2;
        return;
    }
    value->append(text.at(*index));
    ++*index;
}

int readQuoted(const QString &text, int index, QString *path)
{
    QString value;
    while (index < text.size() && text.at(index) != QLatin1Char('"'))
        appendQuoted(text, &index, &value);
    if (index < text.size() && text.at(index) == QLatin1Char('"'))
        ++index;
    *path = value;
    return index;
}

bool takeMode(FileDiff &file, const QString &line)
{
    if (line.startsWith(QLatin1String("new file mode"))) {
        file.added = true;
        return true;
    }
    if (line.startsWith(QLatin1String("deleted file mode"))) {
        file.removed = true;
        return true;
    }
    return false;
}

bool takeRename(FileDiff &file, const QString &line)
{
    if (line.startsWith(QLatin1String("rename from "))) {
        file.oldPath = line.mid(12).trimmed();
        return true;
    }
    if (line.startsWith(QLatin1String("rename to "))) {
        file.newPath = line.mid(10).trimmed();
        return true;
    }
    return false;
}

bool takeCopy(FileDiff &file, const QString &line)
{
    if (line.startsWith(QLatin1String("copy from "))) {
        file.oldPath = line.mid(10).trimmed();
        return true;
    }
    if (line.startsWith(QLatin1String("copy to "))) {
        file.newPath = line.mid(8).trimmed();
        return true;
    }
    return false;
}

bool takePaths(FileDiff &file, const QString &line)
{
    if (line.startsWith(QLatin1String("--- "))) {
        file.oldPath = pathAfter(line);
        return true;
    }
    if (line.startsWith(QLatin1String("+++ "))) {
        file.newPath = pathAfter(line);
        return true;
    }
    return false;
}

} // namespace

QString expandTabs(const QString &text)
{
    QString out;
    out.reserve(text.size());
    int col = 0;
    for (const QChar ch : text) {
        if (ch == QLatin1Char('\t')) {
            const int spaces = 4 - (col % 4);
            out.append(QString(spaces, QLatin1Char(' ')));
            col += spaces;
        } else {
            out.append(ch);
            ++col;
        }
    }
    return out;
}

int readPath(const QString &text, int index, QString *path)
{
    while (index < text.size() && text.at(index).isSpace())
        ++index;
    if (index >= text.size())
        return index;
    if (text.at(index) != QLatin1Char('"'))
        return readPlain(text, index, path);
    return readQuoted(text, index + 1, path);
}

QString pathAfter(const QString &line)
{
    QString rest = line.mid(4);
    const int tab = rest.indexOf(QLatin1Char('\t'));
    if (tab >= 0)
        rest.truncate(tab);
    rest = rest.trimmed();
    if (!rest.startsWith(QLatin1Char('"')))
        return rest;
    QString path;
    readPath(rest, 0, &path);
    return path;
}

bool takeHeader(ParseCursor &cur, const QString &line)
{
    if (takeMode(cur.file, line) || takeRename(cur.file, line) || takeCopy(cur.file, line))
        return true;
    if (takePaths(cur.file, line))
        return true;
    if (!line.startsWith(QLatin1String("GIT binary patch")) && !line.startsWith(QLatin1String("Binary files ")))
        return false;
    cur.file.binary = true;
    cur.skipping = true;
    cur.inHunk = false;
    cur.lines.clear();
    return true;
}
