// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"

#include "LangTune.hpp"

namespace {

bool hereDef(const QList<LangDef> &hits, const LangSpot &spot)
{
    for (const LangDef &def : hits) {
        if (def.path == spot.path && def.line == spot.line)
            return true;
    }
    return false;
}

} // namespace

bool langReady(const QString &root, const QString &path)
{
    const LangTune *tune = langTune(langFor(path));
    if (!tune || !tune->note)
        return true;
    return langCached(root);
}

LangSpot defSpot(const QString &root, const QString &path, int line, int column, const QString &name,
                 const QString &receiver)
{
    LangSpot spot;
    spot.root = root;
    spot.path = path;
    spot.line = line;
    spot.column = column;
    spot.name = name;
    spot.receiver = receiver;
    return spot;
}

LangDef lookupDef(const LangSpot &spot)
{
    const LangTune *tune = langTune(langFor(spot.path));
    if (!tune || spot.name.isEmpty() || spot.root.isEmpty())
        return {};
    LangDef extra;
    if (tune->extra && tune->extra(spot, &extra))
        return extra;
    langEnsure(spot.root, spot.name);
    const QList<LangDef> hits = langHits(spot.root, spot.name, tune->lang);
    if (hereDef(hits, spot))
        return {};
    if (tune->choose)
        return tune->choose(hits, spot);
    return langPick(hits, spot.path);
}
