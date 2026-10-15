// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

namespace {

QString camelTail(const QString &word)
{
    QString out;
    bool upper = true;
    for (const QChar ch : word) {
        if (ch == QLatin1Char('_')) {
            upper = true;
            continue;
        }
        out += upper ? ch.toUpper() : ch;
        upper = false;
    }
    return out;
}

bool callWord(const QString &text, int at, QLatin1String key)
{
    if (at < 0 || !text.mid(at).startsWith(key))
        return false;
    return at == 0 || !rubyWordChar(text.at(at - 1));
}

QString parenWord(const QString &text, int from)
{
    const int end = rubyWordEnd(text, from);
    const int paren = rubySkip(text, end);
    if (end == from || paren >= text.size() || text.at(paren) != QLatin1Char('('))
        return {};
    return text.mid(from, end - from);
}

QString createTail(const QString &text, int at)
{
    const QString key = QStringLiteral("create_");
    if (!callWord(text, at, QLatin1String("create_")))
        return {};
    return camelTail(parenWord(text, at + key.size()));
}

} // namespace

// create_histogram wires the method to the module prepended onto Histogram.
void rubyFactory(const QString &root, const QString &text, QStringList *types)
{
    if (!types->isEmpty())
        return;
    const QString key = QStringLiteral("create_");
    for (int from = 0; from < text.size();) {
        const int at = text.indexOf(key, from);
        if (at < 0)
            return;
        const QString tail = createTail(text, at);
        from = at + key.size();
        if (tail.isEmpty())
            continue;
        langEnsure(root, tail);
        for (const QString &mod : rubyMixed(tail)) {
            if (!types->contains(mod))
                types->append(mod);
        }
    }
}
