// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDef.hpp"

#include "RubyDetail.hpp"

namespace {

bool classReceiver(const QString &name)
{
    return !name.isEmpty() && name.at(0).isUpper();
}

bool ownerMatch(const QString &owner, const QString &receiver)
{
    if (owner.isEmpty())
        return false;
    if (owner == receiver || owner.endsWith(QStringLiteral("::") + receiver))
        return true;
    return receiver.endsWith(QStringLiteral("::") + owner);
}

QList<LangDef> ownedHits(const QList<LangDef> &hits, const QString &receiver)
{
    QList<LangDef> owned;
    for (const LangDef &def : hits) {
        if (ownerMatch(def.owner, receiver))
            owned.append(def);
    }
    return owned;
}

LangDef selfCall(const QList<LangDef> &defs, const QString &path)
{
    QList<LangDef> selves;
    for (const LangDef &def : defs) {
        if (def.signature.contains(QLatin1String("def self.")))
            selves.append(def);
    }
    return rubyPick(selves.isEmpty() ? defs : selves, path);
}

} // namespace

LangDef rubyLookup(const LangSpot &spot)
{
    const LangDef column = schemaFind(spot.root, spot.receiver, spot.name);
    if (column.found() && (column.path != spot.path || column.line != spot.line))
        return column;
    const QList<LangDef> hits = rubyHits(spot.root, spot.name);
    for (const LangDef &def : hits) {
        if (def.path == spot.path && def.line == spot.line)
            return {};
    }
    if (!classReceiver(spot.receiver))
        return rubyPick(hits, spot.path);
    return selfCall(ownedHits(hits, spot.receiver), spot.path);
}
