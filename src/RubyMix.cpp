// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangTune.hpp"
#include "RubyDetail.hpp"

#include <algorithm>

namespace {

bool ownerIs(const QString &owner, const QString &want)
{
    if (owner.isEmpty() || want.isEmpty())
        return false;
    if (owner == want || owner.endsWith(QStringLiteral("::") + want))
        return true;
    return want.endsWith(QStringLiteral("::") + owner);
}

bool sameDef(const LangDef &def, const QString &path, int line)
{
    return def.path == path && def.line == line;
}

bool listed(const QList<LangDef> &out, const LangDef &def)
{
    for (const LangDef &have : out) {
        if (have.path == def.path && have.line == def.line)
            return true;
    }
    return false;
}

QList<LangDef> ownedBy(const QList<LangDef> &hits, const QString &owner, const QString &path, int defLine)
{
    QList<LangDef> out;
    for (const LangDef &def : hits) {
        if (!sameDef(def, path, defLine) && ownerIs(def.owner, owner))
            out.append(def);
    }
    return out;
}

QString classParent(const QString &root, const QString &name)
{
    langEnsure(root, name);
    for (const LangDef &def : langHits(root, name, Lang::Ruby)) {
        const QString parent = rubyParentName(def.signature);
        if (!parent.isEmpty())
            return parent;
    }
    return {};
}

QString hitBlock(const LangDef &def)
{
    QString block = def.path + QLatin1Char(':') + QString::number(def.line) + QLatin1Char('\n') + def.signature;
    if (!def.body.isEmpty())
        block += QLatin1Char('\n') + def.body;
    return block;
}

} // namespace

LangDef joined(QList<LangDef> hits)
{
    if (hits.size() <= 1)
        return hits.isEmpty() ? LangDef() : hits.first();
    std::sort(hits.begin(), hits.end(), [](const LangDef &left, const LangDef &right) {
        return left.path == right.path ? left.line < right.line : left.path < right.path;
    });
    QStringList blocks;
    for (const LangDef &def : hits)
        blocks.append(hitBlock(def));
    LangDef top = hits.first();
    top.comment.clear();
    top.signature = blocks.join(QStringLiteral("\n\n"));
    top.body.clear();
    return top;
}

LangDef ancestorDef(const QString &root, const QString &parent, const QList<LangDef> &hits, const QString &path, int defLine)
{
    QString name = parent;
    for (int step = 0; step < 6 && !name.isEmpty(); ++step) {
        const QList<LangDef> owned = ownedBy(hits, name, path, defLine);
        if (!owned.isEmpty())
            return joined(owned);
        name = classParent(root, name);
    }
    return {};
}

QList<LangDef> hostDefs(const QString &root, const QString &module, const QList<LangDef> &hits, const QString &path, int defLine)
{
    QList<LangDef> out;
    for (const QString &host : rubyHosts(module)) {
        for (const LangDef &def : ownedBy(hits, host, path, defLine)) {
            if (!listed(out, def))
                out.append(def);
        }
    }
    if (!out.isEmpty())
        return out;
    for (const QString &host : rubyHosts(module)) {
        const LangDef def = ancestorDef(root, classParent(root, host), hits, path, defLine);
        if (def.found() && !listed(out, def))
            out.append(def);
    }
    return out;
}
