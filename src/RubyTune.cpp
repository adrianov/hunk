// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangTune.hpp"
#include "RubyDef.hpp"
#include "RubyDetail.hpp"

namespace {

bool hashKey(const QString &text, int end)
{
    if (end >= text.size() || text.at(end) != QLatin1Char(':'))
        return false;
    return end + 1 >= text.size() || text.at(end + 1) != QLatin1Char(':');
}

QString typeAt(const QString &text, int start, int end)
{
    const QString constant = rubyConst(text, start, end);
    if (!constant.isEmpty() || start >= end || text.at(start) != QLatin1Char(':'))
        return constant;
    return text.mid(start + 1, end - start - 1);
}

QString rubySpan(const QString &text, int start, int end, SynKind kind)
{
    if (hashKey(text, end))
        return {};
    if (kind == SynKind::Keyword)
        return text.mid(start, end - start) == QLatin1String("super") ? QStringLiteral("super") : QString();
    if (kind == SynKind::Type)
        return typeAt(text, start, end);
    return text.mid(start, end - start);
}

void rubyReset(int ticket)
{
    schemaBump(ticket);
    rubyResetMix();
    rubyForgetLines();
}

} // namespace

const LangTune *rubyLayer()
{
    static const LangTune tune = {
        Lang::Ruby, true, true, true, true, rubySpan, rubySeek, rubyExtra, rubyChoose, noteRuby, rubyUses, rubyReset,
    };
    return &tune;
}
