// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "SyntaxRule.hpp"

namespace {

int skipSpace(const QString &text, int index)
{
    while (index < text.size() && text.at(index).isSpace())
        ++index;
    return index;
}

int keyStart(const QString &text, int lead)
{
    return QStringView(text).sliced(lead).startsWith(QLatin1String("export ")) ? skipSpace(text, lead + 7) : lead;
}

int keyEnd(const QString &text, int index, const Rule &rule)
{
    while (index < text.size() && (wordChar(text.at(index), rule) || text.at(index) == QLatin1Char('.')))
        ++index;
    return index;
}

// Start of the value after "=", or -1 when the key has no "=".
int valueAt(const QString &text, int end)
{
    const int eq = skipSpace(text, end);
    return eq < text.size() && text.at(eq) == QLatin1Char('=') ? skipSpace(text, eq + 1) : -1;
}

// An unquoted value runs to the end, or to a " #" comment. A quoted one is left to the string scan.
int eatValue(const QString &text, int index, QList<SynSpan> *out)
{
    if (index >= text.size())
        return index;
    if (text.at(index) == QLatin1Char('"') || text.at(index) == QLatin1Char('\''))
        return index;
    const int hash = text.indexOf(QLatin1String(" #"), index);
    const int end = hash < 0 ? text.size() : hash;
    addSpan(out, index, end, SynKind::String);
    addSpan(out, end + 1, text.size(), SynKind::Comment);
    return text.size();
}

} // namespace

// "export KEY=value" at the start of a .env line: the key reads as a constant, the value as a string.
int eatEnv(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if ((rule.flags & Env) == 0 || index != 0)
        return index;
    const int lead = skipSpace(text, 0);
    const int start = keyStart(text, lead);
    const int end = keyEnd(text, start, rule);
    const int value = valueAt(text, end);
    if (end == start || value < 0)
        return index;
    if (start > lead)
        addSpan(out, lead, lead + 6, SynKind::Keyword);
    addSpan(out, start, end, SynKind::Type);
    return eatValue(text, value, out);
}
