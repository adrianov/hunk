// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

#include <QDir>
#include <QFile>

namespace {

bool plainPath(const QString &path)
{
    for (const QChar ch : path) {
        if (ch.isSpace() || ch == QLatin1Char('"') || ch == QLatin1Char('\\'))
            return false;
    }
    return true;
}

QString quoteBody(const QString &path)
{
    QString out;
    for (const QChar ch : path) {
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\\'))
            out += QLatin1Char('\\');
        out += ch;
    }
    return out;
}

QString shownPath(const QString &path)
{
    if (plainPath(path))
        return path;
    return QLatin1Char('"') + quoteBody(path) + QLatin1Char('"');
}

QString gitHeader(const QString &shown)
{
    return QStringLiteral("diff --git ") + shown + QLatin1Char(' ') + shown
        + QStringLiteral("\nnew file mode 100644\n");
}

QString textHead(const QString &shown)
{
    return gitHeader(shown) + QStringLiteral("--- /dev/null\n+++ ") + shown + QLatin1Char('\n');
}

QStringList plusRows(const QString &body)
{
    QStringList rows = body.split(QLatin1Char('\n'));
    if (body.endsWith(QLatin1Char('\n')))
        rows.removeLast();
    return rows;
}

QString hunkHead(int count)
{
    return QStringLiteral("@@ -0,0 +1,") + QString::number(count) + QStringLiteral(" @@");
}

void appendPlus(QStringList *lines, const QStringList &rows)
{
    for (const QString &row : rows)
        lines->append(QLatin1Char('+') + row);
}

QString plusBody(const QString &body)
{
    if (body.isEmpty())
        return {};
    const QStringList rows = plusRows(body);
    QStringList lines{hunkHead(rows.size())};
    appendPlus(&lines, rows);
    if (!body.endsWith(QLatin1Char('\n')))
        lines << QStringLiteral("\\ No newline at end of file");
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString binaryNote(const QString &path)
{
    const QString shown = shownPath(path);
    return gitHeader(shown) + QStringLiteral("Binary files /dev/null and ") + shown + QStringLiteral(" differ\n");
}

QString readDiff(const QString &path, QFile *file)
{
    if (file->peek(8192).contains('\0'))
        return binaryNote(path);
    return textHead(shownPath(path)) + plusBody(QString::fromUtf8(file->readAll()));
}

QString oneFile(const QString &root, const QString &path)
{
    QFile file(QDir(root).filePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return readDiff(path, &file);
}

} // namespace

void addUntracked(GitResult *result)
{
    if (!result->worktree || result->untracked.isEmpty())
        return;
    QString extra;
    for (const QString &path : result->untracked)
        extra += oneFile(result->root, path);
    if (extra.isEmpty())
        return;
    if (!result->diffText.isEmpty() && !result->diffText.endsWith(QLatin1Char('\n')))
        result->diffText += QLatin1Char('\n');
    result->diffText += extra;
}
