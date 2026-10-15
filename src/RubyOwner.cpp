// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

namespace {

QString callType(const QString &text, QString *word)
{
    int from = 0;
    while (from < text.size()) {
        const int dot = text.indexOf(QLatin1Char('.'), from);
        if (dot < 0)
            return {};
        const QString name = wordAt(text, rubySkip(text, dot + 1));
        const QString type = constBefore(text, dot);
        if (!type.isEmpty() && !name.isEmpty() && name.at(0).isLower()) {
            *word = name;
            return type;
        }
        from = dot + 1;
    }
    return {};
}

LangDef ownedDef(const QString &root, const QString &name, const QString &owner)
{
    langEnsure(root, name);
    LangDef found;
    int count = 0;
    for (const LangDef &def : langHits(root, name, Lang::Ruby)) {
        if (!rubyOwns(def.owner, owner) || !defSig(def))
            continue;
        found = def;
        ++count;
    }
    return count == 1 ? found : LangDef();
}

QString parentOf(const QString &root, const QString &name)
{
    langEnsure(root, name);
    for (const LangDef &def : langHits(root, name, Lang::Ruby)) {
        const QString parent = rubyParentName(def.signature);
        if (!parent.isEmpty())
            return parent;
    }
    return {};
}

QString definingOwner(const QString &root, const QString &type, const QString &method)
{
    langEnsure(root, method);
    QString found;
    for (const LangDef &def : langHits(root, method, Lang::Ruby)) {
        if (!defSig(def) || !rubyOwns(def.owner, type))
            continue;
        if (!found.isEmpty() && found != def.owner)
            return {};
        found = def.owner;
    }
    return found;
}

QString methodOwner(const QString &root, const QString &type, const QString &method)
{
    QString name = type;
    for (int step = 0; step < 6 && !name.isEmpty(); ++step) {
        const QString owner = definingOwner(root, name, method);
        if (!owner.isEmpty())
            return owner;
        name = parentOf(root, name);
    }
    return {};
}

QString qualify(const QString &root, const QString &nest, const QString &name)
{
    if (name.isEmpty() || name.contains(QLatin1String("::")))
        return name;
    const QString full = nest.isEmpty() ? name : nest + QLatin1String("::") + name;
    langEnsure(root, full);
    return langHits(root, full, Lang::Ruby).isEmpty() ? name : full;
}

QString newOwners(const QString &root, const QString &text, const QString &nest, const QString &method)
{
    QString found;
    int from = 0;
    while (from < text.size()) {
        const int dot = text.indexOf(QLatin1Char('.'), from);
        if (dot < 0)
            return found;
        const QString word = wordAt(text, rubySkip(text, dot + 1));
        if (word == QLatin1String("new")
            && !keepOwner(&found, methodOwner(root, qualify(root, nest, constBefore(text, dot)), method)))
            return {};
        from = dot + 1;
    }
    return found;
}

} // namespace

QString followText(const QString &root, const QString &text, const QString &nest, const QString &method)
{
    const QString direct = newOwners(root, text, nest, method);
    if (!direct.isEmpty())
        return direct;
    QString word;
    const QString type = callType(text, &word);
    if (type.isEmpty())
        return {};
    const LangDef def = ownedDef(root, word, type);
    if (!def.found())
        return {};
    return newOwners(root, def.signature + QLatin1Char('\n') + def.body, type, method);
}
