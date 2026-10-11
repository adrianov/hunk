// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDef.hpp"

#include "RubyDetail.hpp"

LangDef rubyLookup(const LangSpot &spot)
{
    const QList<LangDef> hits = rubyHits(spot.root, spot.name);
    for (const LangDef &def : hits) {
        if (def.path == spot.path && def.line == spot.line)
            return {};
    }
    return rubyPick(hits, spot.path);
}
