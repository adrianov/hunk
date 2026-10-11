// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>
#include <QMutex>

namespace {

QMutex g_mutex;
QString g_key;
QStringList g_lines;

bool endLine(const QString &line)
{
    const int at = rubySkip(line, 0);
    if (!line.mid(at).startsWith(QLatin1String("end")))
        return false;
    const int after = at + 3;
    return after >= line.size() || !rubyWordChar(line.at(after));
}

int defAt(const QStringList &lines, int line)
{
    int limit = rubyIndent(lines.at(line - 1));
    for (int at = line; at >= 1; --at) {
        const QString text = lines.at(at - 1);
        if (text.isEmpty() || rubyIndent(text) > limit)
            continue;
        limit = rubyIndent(text);
        QString name;
        if (rubyDefName(text, &name))
            return at;
        if (rubyTypeName(text, &name))
            return 0;
    }
    return 0;
}

int methodEnd(const QStringList &lines, int defLine)
{
    if (rubyClosed(lines.at(defLine - 1)))
        return defLine;
    const int indent = rubyIndent(lines.at(defLine - 1));
    for (int at = defLine + 1; at <= lines.size(); ++at) {
        if (rubyIndent(lines.at(at - 1)) <= indent && endLine(lines.at(at - 1)))
            return at;
    }
    return lines.size();
}

bool methodAssigns(const QStringList &lines, int defLine, const QString &name)
{
    for (int at = defLine, last = methodEnd(lines, defLine); at <= last; ++at) {
        if (rubyAssigned(lines.at(at - 1), name))
            return true;
    }
    return false;
}

enum class Token { Miss, Call, Bare };

bool called(const QString &line, int at, int end)
{
    if (at > 0 && line.at(at - 1) == QLatin1Char('.'))
        return true;
    const int next = rubySkip(line, end);
    return next < line.size() && line.at(next) == QLatin1Char('(');
}

Token tokenAt(const QString &line, int column, const QString &name)
{
    int from = 0;
    while (from < line.size()) {
        const int at = line.indexOf(name, from);
        if (at < 0)
            return Token::Miss;
        const int end = at + name.size();
        if (rubyWordHit(line, at, name) && column >= at && column < end)
            return called(line, at, end) ? Token::Call : Token::Bare;
        from = qMax(end, at + 1);
    }
    return Token::Miss;
}

QStringList fileLines(const QString &root, const QString &path)
{
    const QString key = root + QLatin1Char('\n') + path;
    {
        const QMutexLocker lock(&g_mutex);
        if (g_key == key)
            return g_lines;
    }
    QFile file(QDir(root).filePath(path));
    QStringList lines;
    if (file.open(QIODevice::ReadOnly))
        lines = rubyLines(QString::fromUtf8(file.readAll()));
    const QMutexLocker lock(&g_mutex);
    g_key = key;
    g_lines = lines;
    return g_lines;
}

} // namespace

void rubyForgetLocal()
{
    const QMutexLocker lock(&g_mutex);
    g_key.clear();
    g_lines.clear();
}

bool rubyLocal(const LangSpot &spot)
{
    if (spot.line < 1 || spot.name.isEmpty() || spot.name.at(0).isUpper() || spot.name.startsWith(QLatin1Char('@')))
        return false;
    const QStringList lines = fileLines(spot.root, spot.path);
    if (spot.line > lines.size() || tokenAt(lines.at(spot.line - 1), spot.column, spot.name) != Token::Bare)
        return false;
    const int defLine = defAt(lines, spot.line);
    return defLine > 0 && methodAssigns(lines, defLine, spot.name);
}
