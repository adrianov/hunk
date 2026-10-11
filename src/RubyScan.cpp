// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool nameChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('?') || ch == QLatin1Char('!');
}

bool afterDot(const QString &line, int at, QString *name)
{
    at = rubySkip(line, at);
    if (at >= line.size() || line.at(at) != QLatin1Char('.'))
        return true;
    return rubyWord(line, rubySkip(line, at + 1), name, &at);
}

} // namespace

int rubySkip(const QString &line, int at)
{
    while (at < line.size() && line.at(at).isSpace())
        ++at;
    return at;
}

bool rubyWord(const QString &line, int at, QString *word, int *next)
{
    if (at >= line.size() || (!line.at(at).isLetter() && line.at(at) != QLatin1Char('_')))
        return false;
    int end = at + 1;
    while (end < line.size() && nameChar(line.at(end)))
        ++end;
    *word = line.mid(at, end - at);
    *next = end;
    return true;
}

bool rubyDefName(const QString &line, QString *name)
{
    int at = rubySkip(line, 0);
    if (!line.mid(at).startsWith(QLatin1String("def")))
        return false;
    at += 3;
    if (at < line.size() && (line.at(at).isLetterOrNumber() || line.at(at) == QLatin1Char('_')))
        return false;
    int next = 0;
    if (!rubyWord(line, rubySkip(line, at), name, &next))
        return false;
    return afterDot(line, next, name);
}

QStringList rubyLines(const QString &text)
{
    QStringList lines = text.split(QLatin1Char('\n'));
    for (QString &line : lines) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
    }
    return lines;
}

QString rubyComment(const QStringList &lines, int line)
{
    QStringList notes;
    for (int at = line - 1; at >= 1; --at) {
        const QString trimmed = lines.at(at - 1).trimmed();
        if (trimmed.isEmpty()) {
            if (notes.isEmpty())
                continue;
            break;
        }
        if (!trimmed.startsWith(QLatin1Char('#')))
            break;
        notes.prepend(lines.at(at - 1));
    }
    return notes.join(QLatin1Char('\n'));
}
