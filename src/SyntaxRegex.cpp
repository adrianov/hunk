// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "SyntaxRule.hpp"

namespace {

bool divideAfter(QChar ch)
{
    return ch.isLetterOrNumber() || QStringLiteral("_)]}<").contains(ch);
}

// A "/" after a value is division, after "<" a closing tag, and "/>" or "/=" never a regex.
// Only "scan /x/", with a space before the slash and none after, opens one after a value.
bool regexStart(const QString &text, int index)
{
    const QStringView head = QStringView(text).left(index).trimmed();
    if (head.isEmpty() || !divideAfter(head.back()))
        return true;
    return text.at(index - 1).isSpace() && !QStringLiteral(" \t=>").contains(text.mid(index + 1, 1));
}

// Index after the closing "/", or -1 when the line has none. A "/" inside [...] does not close.
int regexEnd(const QString &text, int index)
{
    bool inClass = false;
    for (int cursor = index + 1; cursor < text.size(); ++cursor) {
        const QChar ch = text.at(cursor);
        if (ch == QLatin1Char('\\'))
            ++cursor;
        else if (ch == QLatin1Char('[') || ch == QLatin1Char(']'))
            inClass = ch == QLatin1Char('[');
        else if (ch == QLatin1Char('/') && !inClass)
            return cursor + 1;
    }
    return -1;
}

} // namespace

// A Ruby or JavaScript regex literal such as /id="([^"]+)"/i, so its quotes do not open a string.
int eatRegex(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if ((rule.flags & Regex) == 0 || text.at(index) != QLatin1Char('/') || !regexStart(text, index))
        return index;
    int end = regexEnd(text, index);
    if (end < 0)
        return index;
    while (end < text.size() && text.at(end).isLetter())
        ++end;
    addSpan(out, index, end, SynKind::String);
    return end;
}
