// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool plainAssign(const QString &line, int at)
{
    at = rubySkip(line, at);
    if (at >= line.size() || line.at(at) != QLatin1Char('='))
        return false;
    return at + 1 >= line.size() || !QStringLiteral("=>~").contains(line.at(at + 1));
}

} // namespace

bool rubyAssigned(const QString &line, const QString &name)
{
    const int hash = line.indexOf(QLatin1Char('#'));
    int from = 0;
    while (from < line.size()) {
        const int at = line.indexOf(name, from);
        if (at < 0)
            return false;
        const int end = at + name.size();
        if (rubyWordHit(line, at, name) && (hash < 0 || at < hash) && plainAssign(line, end))
            return true;
        from = qMax(end, at + 1);
    }
    return false;
}
