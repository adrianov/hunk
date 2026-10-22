// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "SyntaxRule.hpp"

namespace {

// Color the code after "#{" and return the index of the "}" that closes it.
int codeEnd(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    for (int depth = 0; index < text.size();) {
        if (text.at(index) == '}' && depth-- == 0)
            break;
        depth += text.at(index) == '{';
        const int next = takeToken(text, index, rule, scan, out);
        index = next == index ? index + 1 : next;
    }
    return index;
}

// Index of the closing quote or of the next "#{" in a Ruby string.
int partEnd(const QString &text, int cursor)
{
    for (; cursor < text.size() && text.at(cursor) != '"'; ++cursor) {
        if (text.at(cursor) == '\\')
            ++cursor;
        else if (QStringView(text).sliced(cursor).startsWith(u"#{"))
            break;
    }
    return cursor;
}

// Color the string from the opening quote or a closing "}" at start, through the closing quote.
int stringRest(const QString &text, int start, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    const int cursor = partEnd(text, start + 1);
    if (text.mid(cursor, 1) != u"#") {
        const int end = qMin(cursor + 1, int(text.size()));
        addSpan(out, start, end, SynKind::String);
        return end;
    }
    addSpan(out, start, cursor + 2, SynKind::String);
    return stringRest(text, codeEnd(text, cursor + 2, rule, scan, out), rule, scan, out);
}

} // namespace

// Ruby "a #{code} b" colors the code between the braces as code, so hover and Command-click reach it.
int eatInterp(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    if ((rule.flags & RubyId) == 0 || text.at(index) != '"')
        return index;
    return stringRest(text, index, rule, scan, out);
}
