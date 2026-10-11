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

int beforeBar(const QString &line, int bar)
{
    while (bar > 0 && line.at(bar - 1).isSpace())
        --bar;
    return bar;
}

bool blockOpen(const QString &line, int bar)
{
    const int at = beforeBar(line, bar);
    if (at > 0 && line.at(at - 1) == QLatin1Char('{'))
        return true;
    return at >= 2 && line.mid(at - 2, 2) == QLatin1String("do") && (at == 2 || !rubyWordChar(line.at(at - 3)));
}

bool inBlock(const QString &line, int at)
{
    const int open = line.lastIndexOf(QLatin1Char('|'), at > 0 ? at - 1 : 0);
    if (open < 0 || open >= at)
        return false;
    return line.indexOf(QLatin1Char('|'), at) > at && blockOpen(line, open);
}

} // namespace

bool rubyBlockArg(const QString &line, const QString &name)
{
    const int hash = line.indexOf(QLatin1Char('#'));
    int from = 0;
    while (from < line.size()) {
        const int at = line.indexOf(name, from);
        if (at < 0 || (hash >= 0 && at > hash))
            return false;
        const int end = at + name.size();
        if (rubyWordHit(line, at, name) && inBlock(line, at))
            return true;
        from = qMax(end, at + 1);
    }
    return false;
}

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
