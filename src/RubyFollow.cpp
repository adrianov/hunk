// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

namespace {

bool nameOn(const QString &line, const QString &name)
{
    return rubyAssigned(line, name) || rubyBlockArg(line, name);
}

bool paramOn(const QString &line, const QString &name)
{
    const int open = line.indexOf(QLatin1Char('('));
    const int close = line.indexOf(QLatin1Char(')'), open);
    const int at = line.indexOf(name, open);
    if (open < 0 || close < 0 || at < 0 || at > close)
        return false;
    return rubyWordHit(line, at, name);
}

bool shadowed(const LangSpot &spot, const RubyBound &bound)
{
    if (bound.opened <= 0)
        return false;
    if (paramOn(bound.lines.at(bound.opened - 1), spot.receiver))
        return true;
    for (int at = bound.opened; at <= bound.last; ++at) {
        if (nameOn(bound.lines.at(at - 1), spot.receiver))
            return true;
    }
    return false;
}

LangDef fileDef(const LangSpot &spot)
{
    langEnsure(spot.root, spot.receiver);
    LangDef found;
    int count = 0;
    for (const LangDef &def : langHits(spot.root, spot.receiver, Lang::Ruby)) {
        if (def.path != spot.path || !defSig(def))
            continue;
        found = def;
        ++count;
    }
    return count == 1 ? found : LangDef();
}

} // namespace

// settings.precision_for opens precision_for on the class settings returns.
QString rubyFollow(const LangSpot &spot, const RubyBound &bound)
{
    if (shadowed(spot, bound))
        return {};
    const LangDef def = fileDef(spot);
    if (!def.found())
        return {};
    return followText(spot.root, def.signature + QLatin1Char('\n') + def.body, def.owner, spot.name);
}
