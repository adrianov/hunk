// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

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
        if (rubyIndent(lines.at(at - 1)) <= indent && rubyEndLine(lines.at(at - 1)))
            return at;
    }
    return lines.size();
}

int blockEnd(const QStringList &lines, int open, int last)
{
    const int indent = rubyIndent(lines.at(open - 1));
    for (int at = open + 1; at <= last; ++at) {
        const QString text = lines.at(at - 1);
        if (text.isEmpty() || rubyIndent(text) > indent)
            continue;
        return rubyEndLine(text) ? at : open;
    }
    return open;
}

bool methodAssigns(const QStringList &lines, int defLine, int hover, const QString &name)
{
    const int last = methodEnd(lines, defLine);
    for (int at = defLine; at <= last; ++at) {
        const QString text = lines.at(at - 1);
        if (rubyAssigned(text, name))
            return true;
        if (rubyBlockArg(text, name) && hover >= at && hover <= blockEnd(lines, at, last))
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

} // namespace

bool rubyLocal(const LangSpot &spot)
{
    if (spot.line < 1 || spot.name.isEmpty() || spot.name.at(0).isUpper() || spot.name.startsWith(QLatin1Char('@')))
        return false;
    const QStringList lines = linesFor(spot.root, spot.path);
    if (spot.line > lines.size() || tokenAt(lines.at(spot.line - 1), spot.column, spot.name) != Token::Bare)
        return false;
    const int defLine = defAt(lines, spot.line);
    return defLine > 0 && methodAssigns(lines, defLine, spot.line, spot.name);
}
