// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool inheritAt(const QString &line, int *at)
{
    const int lt = line.indexOf(QLatin1Char('<'));
    if (lt < 0 || lt + 1 >= line.size())
        return false;
    const QChar next = line.at(lt + 1);
    if (next == QLatin1Char('<') || next == QLatin1Char('='))
        return false;
    *at = rubySkip(line, lt + 1);
    return true;
}

int wordEnd(const QString &line, int at)
{
    while (at < line.size() && (line.at(at).isLetterOrNumber() || line.at(at) == QLatin1Char('_')))
        ++at;
    return at;
}

} // namespace

QString rubyParentName(const QString &line)
{
    int at = 0;
    if (!inheritAt(line, &at))
        return {};
    return rubyConst(line, at, wordEnd(line, at));
}
