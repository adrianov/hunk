// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "SyntaxRule.hpp"

namespace {

bool quoteChar(QChar ch)
{
    return ch == QLatin1Char('\'') || ch == QLatin1Char('"') || ch == QLatin1Char('`');
}

int stringEnd(const QString &text, int index, QChar quote)
{
    int cursor = index + 1;
    while (cursor < text.size()) {
        if (text.at(cursor) == QLatin1Char('\\')) {
            cursor += 2;
            continue;
        }
        if (text.at(cursor) == quote)
            return cursor + 1;
        ++cursor;
    }
    return text.size();
}

bool tripleAt(const QString &text, int index, QChar *quote)
{
    if (index + 2 >= text.size())
        return false;
    const QChar ch = text.at(index);
    if (ch != QLatin1Char('\'') && ch != QLatin1Char('"'))
        return false;
    if (text.at(index + 1) != ch || text.at(index + 2) != ch)
        return false;
    *quote = ch;
    return true;
}

bool numberChar(const QString &text, int cursor)
{
    const QChar ch = text.at(cursor);
    if (ch == QLatin1Char('.') && cursor + 1 < text.size() && text.at(cursor + 1) == QLatin1Char('.'))
        return false;
    return ch.isDigit() || ch == QLatin1Char('.') || ch == QLatin1Char('_');
}

int numberEnd(const QString &text, int index)
{
    int cursor = index + 1;
    while (cursor < text.size() && numberChar(text, cursor))
        ++cursor;
    return cursor;
}

bool hexDigit(QChar ch)
{
    if (ch.isDigit())
        return true;
    return (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')) || (ch >= QLatin1Char('A') && ch <= QLatin1Char('F'));
}

bool wordApos(const QString &text, int index)
{
    if (index <= 0)
        return false;
    const QChar prev = text.at(index - 1);
    return prev.isLetterOrNumber() || prev == QLatin1Char('_');
}

} // namespace

void addSpan(QList<SynSpan> *out, int start, int end, SynKind kind)
{
    if (end > start)
        out->append(SynSpan{start, end, kind});
}

int eatString(const QString &text, int index, QList<SynSpan> *out)
{
    const QChar quote = text.at(index);
    if (!quoteChar(quote) || (quote == QLatin1Char('\'') && wordApos(text, index)))
        return index;
    const int end = stringEnd(text, index, quote);
    addSpan(out, index, end, SynKind::String);
    return end;
}

int eatTriple(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    if ((rule.flags & Triple) == 0)
        return index;
    QChar quote;
    if (!tripleAt(text, index, &quote))
        return index;
    const int at = text.indexOf(QString(3, quote), index + 3);
    if (at < 0) {
        addSpan(out, index, text.size(), SynKind::String);
        scan->quote = quote.toLatin1();
        scan->triple = true;
        return text.size();
    }
    addSpan(out, index, at + 3, SynKind::String);
    return at + 3;
}

int eatNumber(const QString &text, int index, QList<SynSpan> *out)
{
    if (!text.at(index).isDigit())
        return index;
    const int end = numberEnd(text, index);
    addSpan(out, index, end, SynKind::Number);
    return end;
}

int eatHex(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if ((rule.flags & Css) == 0 || text.at(index) != QLatin1Char('#'))
        return index;
    int cursor = index + 1;
    while (cursor < text.size() && hexDigit(text.at(cursor)))
        ++cursor;
    if (cursor == index + 1)
        return index;
    addSpan(out, index, cursor, SynKind::Number);
    return cursor;
}

int resumeString(const QString &text, Scan *scan, QList<SynSpan> *out)
{
    const int at = text.indexOf(QString(3, QChar(scan->quote)));
    if (at < 0) {
        addSpan(out, 0, text.size(), SynKind::String);
        return text.size();
    }
    addSpan(out, 0, at + 3, SynKind::String);
    scan->quote = 0;
    scan->triple = false;
    return at + 3;
}
