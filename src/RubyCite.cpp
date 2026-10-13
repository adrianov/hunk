// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"

#include "LangTune.hpp"
#include "RubyDetail.hpp"

namespace {

bool specPath(const QString &path)
{
    return path.startsWith(QLatin1String("spec/")) || path.startsWith(QLatin1String("test/"))
        || path.contains(QLatin1String("/spec/")) || path.contains(QLatin1String("/test/"));
}

bool methodName(const QString &name)
{
    return !name.isEmpty() && !name.at(0).isUpper();
}

bool hasWord(const QString &text, const QString &name)
{
    int from = 0;
    while (from < text.size()) {
        const int at = text.indexOf(name, from);
        if (at < 0)
            return false;
        if (rubyWordHit(text, at, name))
            return true;
        from = at + 1;
    }
    return false;
}

bool namesOwner(const QString &text, const QString &owner)
{
    if (hasWord(text, owner))
        return true;
    const int cut = owner.lastIndexOf(QLatin1String("::"));
    return cut >= 0 && hasWord(text, owner.mid(cut + 2));
}

bool ownedUse(const LangDef &use, const LangDef &def, const QString &name)
{
    if (!methodName(name) || def.owner.isEmpty() || use.path == def.path)
        return true;
    return namesOwner(use.signature, def.owner) || namesOwner(use.body, def.owner);
}

int useRank(const LangDef &use, const LangDef &def, const QString &name)
{
    if (use.path == def.path && use.line == def.line)
        return 0;
    if (!ownedUse(use, def, name))
        return 0;
    if (specPath(use.path))
        return 1;
    int score = use.path == def.path ? 2 : 4;
    if (methodName(name) && use.signature.contains(QStringLiteral(".") + name))
        score += 2;
    return score;
}

LangDef bestUse(const QList<LangDef> &uses, const LangDef &def, const QString &name, bool onlyNew)
{
    LangDef best;
    int bestScore = 0;
    for (const LangDef &use : uses) {
        if (onlyNew && !use.signature.contains(QLatin1String(".new")))
            continue;
        const int score = useRank(use, def, name);
        if (score <= bestScore)
            continue;
        best = use;
        bestScore = score;
    }
    return best;
}

LangDef defHere(const QList<LangDef> &hits, const LangSpot &spot)
{
    for (const LangDef &def : hits) {
        if (def.path == spot.path && def.line == spot.line)
            return def;
    }
    return {};
}

LangDef mineAt(const LangSpot &spot)
{
    const LangTune *tune = langTune(langFor(spot.path));
    if (!tune || !tune->use || !langSought(spot.root, spot.name))
        return {};
    return defHere(langHits(spot.root, spot.name, tune->lang), spot);
}

LangDef initUse(const LangSpot &spot, const LangDef &mine)
{
    if (mine.owner.isEmpty() || !langSought(spot.root, mine.owner))
        return {};
    return bestUse(langUses(spot.root, mine.owner), mine, {}, true);
}

} // namespace

QString moreUse(const LangSpot &spot)
{
    if (spot.name != QLatin1String("initialize"))
        return {};
    const LangDef mine = mineAt(spot);
    if (!mine.found() || mine.owner.isEmpty() || langSought(spot.root, mine.owner))
        return {};
    return mine.owner;
}

LangDef lookupUse(const LangSpot &spot)
{
    const LangDef mine = mineAt(spot);
    if (!mine.found())
        return {};
    if (spot.name == QLatin1String("initialize"))
        return initUse(spot, mine);
    return bestUse(langUses(spot.root, spot.name), mine, spot.name, false);
}
