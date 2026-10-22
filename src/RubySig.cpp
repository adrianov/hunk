// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QRegularExpression>

namespace {

// The line that closes a heredoc opened on `line`, such as SQL after "<<~SQL.squish", or 0.
int heredocEnd(const QStringList &lines, int line)
{
    static const QRegularExpression open(QStringLiteral("<<[~-]?(['\"]?)([A-Z_][A-Z0-9_]*)\\1"));
    const QString tag = open.match(lines.at(line - 1)).captured(2);
    for (int at = line + 1; !tag.isEmpty() && at <= lines.size(); ++at) {
        if (lines.at(at - 1).trimmed() == tag)
            return at;
    }
    return 0;
}

int skipQuote(const QString &line, int at)
{
    const QChar quote = line.at(at);
    for (++at; at < line.size(); ++at) {
        if (line.at(at) == QLatin1Char('\\'))
            ++at;
        else if (line.at(at) == quote)
            return at + 1;
    }
    return line.size();
}

bool nestOpen(QChar ch)
{
    return ch == QLatin1Char('(') || ch == QLatin1Char('[') || ch == QLatin1Char('{');
}

bool nestClose(QChar ch)
{
    return ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}');
}

int parenDelta(const QString &line)
{
    int depth = 0;
    for (int at = 0; at < line.size(); ++at) {
        const QChar ch = line.at(at);
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\'')) {
            at = skipQuote(line, at) - 1;
            continue;
        }
        if (ch == QLatin1Char('#'))
            break;
        if (nestOpen(ch))
            ++depth;
        else if (nestClose(ch))
            --depth;
    }
    return depth;
}

bool sigDone(const QString &line, int depth)
{
    const QString trimmed = line.trimmed();
    return depth <= 0 && !trimmed.endsWith(QLatin1Char(',')) && !trimmed.endsWith(QLatin1Char('\\'));
}

bool headWord(const QString &line, QLatin1String word)
{
    const int at = rubySkip(line, 0);
    if (!line.mid(at).startsWith(word))
        return false;
    const int end = at + word.size();
    return end >= line.size() || (!line.at(end).isLetterOrNumber() && line.at(end) != QLatin1Char('_'));
}

bool unitLine(const QString &line)
{
    return headWord(line, QLatin1String("def")) || headWord(line, QLatin1String("class"))
        || headWord(line, QLatin1String("module")) || headWord(line, QLatin1String("end"));
}

int nestEnd(const QStringList &lines, int line)
{
    int depth = 0;
    for (int at = line; at <= lines.size(); ++at) {
        const QString text = lines.at(at - 1);
        if (at > line && depth > 0 && unitLine(text))
            return at - 1;
        depth += parenDelta(text);
        if (sigDone(text, depth) || (depth <= 0 && at >= line + 6))
            return at;
    }
    return lines.size();
}

} // namespace

int rubySigEnd(const QStringList &lines, int line)
{
    return qMax(heredocEnd(lines, line), nestEnd(lines, line));
}

QString rubyJoin(const QStringList &lines, int first, int last)
{
    return lines.mid(first - 1, last - first + 1).join(QLatin1Char('\n'));
}
