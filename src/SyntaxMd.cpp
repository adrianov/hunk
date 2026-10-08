// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "SyntaxRule.hpp"

namespace {

bool atStart(const QString &text, int index)
{
    return text.left(index).trimmed().isEmpty();
}

int eatHeading(const QString &text, int index, QList<SynSpan> *out)
{
    if (text.at(index) != QLatin1Char('#'))
        return index;
    int end = index;
    while (end < text.size() && text.at(end) == QLatin1Char('#') && end - index < 6)
        ++end;
    if (end < text.size() && text.at(end) == QLatin1Char('#'))
        return index;
    addSpan(out, index, end, SynKind::Keyword);
    return end;
}

int eatFence(const QString &text, int index, Scan *scan, QList<SynSpan> *out)
{
    const bool tick = text.sliced(index).startsWith(QLatin1String("```"));
    if (!tick && !text.sliced(index).startsWith(QLatin1String("~~~")))
        return index;
    addSpan(out, index, text.size(), SynKind::Keyword);
    scan->triple = true;
    scan->quote = tick ? '`' : '~';
    return text.size();
}

int eatQuote(const QString &text, int index, QList<SynSpan> *out)
{
    if (text.at(index) != QLatin1Char('>'))
        return index;
    addSpan(out, index, index + 1, SynKind::Comment);
    return index + 1;
}

int eatBullet(const QString &text, int index, QList<SynSpan> *out)
{
    const QChar ch = text.at(index);
    const bool mark = ch == QLatin1Char('-') || ch == QLatin1Char('*') || ch == QLatin1Char('+');
    if (!mark || index + 1 >= text.size() || text.at(index + 1) != QLatin1Char(' '))
        return index;
    addSpan(out, index, index + 1, SynKind::Keyword);
    return index + 1;
}

int eatMdBlock(const QString &text, int index, Scan *scan, QList<SynSpan> *out)
{
    if (!atStart(text, index))
        return index;
    if (const int next = eatHeading(text, index, out); next != index)
        return next;
    if (const int next = eatFence(text, index, scan, out); next != index)
        return next;
    if (const int next = eatQuote(text, index, out); next != index)
        return next;
    return eatBullet(text, index, out);
}

int eatLink(const QString &text, int index, QList<SynSpan> *out)
{
    if (text.at(index) != QLatin1Char('['))
        return index;
    const int close = text.indexOf(QLatin1String("]("), index + 1);
    if (close < 0)
        return index;
    const int url = text.indexOf(QLatin1Char(')'), close + 2);
    if (url < 0)
        return index;
    addSpan(out, index + 1, close, SynKind::Method);
    addSpan(out, close + 2, url, SynKind::String);
    return url + 1;
}

int eatCode(const QString &text, int index, QList<SynSpan> *out)
{
    if (text.at(index) != QLatin1Char('`'))
        return index;
    const int end = text.indexOf(QLatin1Char('`'), index + 1);
    if (end < 0)
        return index;
    addSpan(out, index, end + 1, SynKind::String);
    return end + 1;
}

int eatMdInline(const QString &text, int index, QList<SynSpan> *out)
{
    if (const int next = eatLink(text, index, out); next != index)
        return next;
    if (const int next = eatCode(text, index, out); next != index)
        return next;
    return eatEmph(text, index, out);
}

bool fenceClose(const QString &text, int index, QChar quote)
{
    const QString mark(3, quote);
    return text.sliced(index).startsWith(mark) && text.mid(index + mark.size()).trimmed().isEmpty();
}

} // namespace

int resumeFence(const QString &text, Scan *scan, QList<SynSpan> *out)
{
    int index = 0;
    while (index < text.size() && text.at(index).isSpace())
        ++index;
    if (!fenceClose(text, index, QChar(scan->quote))) {
        addSpan(out, 0, text.size(), SynKind::String);
        return text.size();
    }
    addSpan(out, index, index + 3, SynKind::Keyword);
    scan->triple = false;
    scan->quote = 0;
    return index + 3;
}

int eatMarkdown(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    if ((rule.flags & Md) == 0)
        return index;
    if (const int next = eatMdBlock(text, index, scan, out); next != index)
        return next;
    return eatMdInline(text, index, out);
}
