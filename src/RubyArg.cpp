// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool readQuote(const QString &text, int at, RubyArg *group)
{
    const int end = text.indexOf(text.at(at), at + 1);
    if (end <= at + 1)
        return false;
    group->name = text.mid(at + 1, end - at - 1);
    group->start = at;
    group->end = end + 1;
    return true;
}

bool readSymbol(const QString &text, int at, RubyArg *group)
{
    int next = 0;
    if (!rubyWord(text, at + 1, &group->name, &next))
        return false;
    group->start = at;
    group->end = next;
    return true;
}

bool skipMark(QChar ch)
{
    return ch == QLatin1Char('#') || ch == QLatin1Char(',') || ch == QLatin1Char('\\');
}

int afterParen(const QString &text, int at)
{
    at = rubySkip(text, at);
    if (at < text.size() && text.at(at) == QLatin1Char('('))
        return rubySkip(text, at + 1);
    return at;
}

RubyKind argKind(const QString &text, int at, RubyArg *group)
{
    const QChar ch = text.at(at);
    if (ch == QLatin1Char('\'') || ch == QLatin1Char('"'))
        return readQuote(text, at, group) ? RubyKind::Found : RubyKind::None;
    if (ch == QLatin1Char(':'))
        return readSymbol(text, at, group) ? RubyKind::Found : RubyKind::None;
    return RubyKind::None;
}

} // namespace

bool rubySharedWord(const QString &word)
{
    return word == QLatin1String("shared_context") || word == QLatin1String("shared_examples")
        || word == QLatin1String("shared_examples_for");
}

bool rubyIncludeWord(const QString &word)
{
    return word == QLatin1String("include_context") || word == QLatin1String("include_examples")
        || word == QLatin1String("it_behaves_like") || word == QLatin1String("it_should_behave_like");
}

bool rubyHead(const QString &line, QString *word, int *next)
{
    int at = rubySkip(line, 0);
    if (!rubyWord(line, at, word, next))
        return false;
    if (*word != QLatin1String("RSpec"))
        return true;
    at = rubySkip(line, *next);
    if (at >= line.size() || line.at(at) != QLatin1Char('.'))
        return false;
    return rubyWord(line, rubySkip(line, at + 1), word, next);
}

RubyKind rubyTake(const QString &text, int at, RubyArg *group)
{
    at = afterParen(text, at);
    if (at >= text.size() || skipMark(text.at(at)))
        return RubyKind::Wait;
    return argKind(text, at, group);
}

bool rubyOnWord(const QString &line, int column, const QString &word)
{
    const int at = rubySkip(line, 0);
    return column >= at && column < at + word.size() && line.mid(at, word.size()) == word;
}

QStringList rubyFileLines(const QString &root, const QString &path, int line)
{
    if (line < 1 || root.isEmpty() || path.isEmpty())
        return {};
    const QStringList lines = linesFor(root, path);
    return line > lines.size() ? QStringList() : lines;
}
