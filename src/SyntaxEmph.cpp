// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "SyntaxRule.hpp"

namespace {

int runLen(const QString &text, int index, QChar ch)
{
    const int cap = index + (ch == QLatin1Char('~') ? 2 : 3);
    int end = index;
    while (end < text.size() && end < cap && text.at(end) == ch)
        ++end;
    return end - index;
}

bool openEdge(const QString &text, int index, int len)
{
    if (index + len >= text.size() || text.at(index + len).isSpace())
        return false;
    return text.at(index) != QLatin1Char('_') || index == 0 || !text.at(index - 1).isLetterOrNumber();
}

int closeAt(const QString &text, int from, QChar ch, int len)
{
    const QString mark(len, ch);
    int at = from;
    while ((at = text.indexOf(mark, at)) >= 0) {
        const int after = at + len;
        const bool tail = ch != QLatin1Char('_') || after >= text.size() || !text.at(after).isLetterOrNumber();
        if (at > from && !text.at(at - 1).isSpace() && tail)
            return at;
        ++at;
    }
    return -1;
}

SynKind emphKind(QChar ch, int len)
{
    if (ch == QLatin1Char('~'))
        return SynKind::Strike;
    return len == 1 ? SynKind::Emph : SynKind::Strong;
}

bool emphOpen(const QString &text, int index, QChar *ch, int *len)
{
    *ch = text.at(index);
    if (*ch != QLatin1Char('*') && *ch != QLatin1Char('_') && *ch != QLatin1Char('~'))
        return false;
    *len = runLen(text, index, *ch);
    return (*ch != QLatin1Char('~') || *len >= 2) && openEdge(text, index, *len);
}

} // namespace

int eatEmph(const QString &text, int index, QList<SynSpan> *out)
{
    QChar ch;
    int len = 0;
    if (!emphOpen(text, index, &ch, &len))
        return index;
    const int close = closeAt(text, index + len, ch, len);
    if (close < 0)
        return index;
    addSpan(out, index, index + len, SynKind::Keyword);
    addSpan(out, index + len, close, emphKind(ch, len));
    addSpan(out, close, close + len, SynKind::Keyword);
    return close + len;
}
