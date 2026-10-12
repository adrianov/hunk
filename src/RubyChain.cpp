// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

#include <QSet>

namespace {

int wordEnd(const QString &text, int at)
{
    while (at < text.size() && (text.at(at).isLetterOrNumber() || text.at(at) == QLatin1Char('_')))
        ++at;
    return at;
}

bool declLine(const QString &signature)
{
    const QString line = signature.trimmed();
    if (line.startsWith(QLatin1String("def ")) || line.contains(QLatin1String(" def ")))
        return false;
    return !line.startsWith(QLatin1String("class ")) && !line.startsWith(QLatin1String("module "));
}

bool defLine(const QString &signature)
{
    return signature.trimmed().startsWith(QLatin1String("def "));
}

LangDef ownedKind(const QString &root, const QString &owner, const QString &name, bool decl)
{
    langEnsure(root, name);
    for (const LangDef &def : langHits(root, name, Lang::Ruby)) {
        const bool kind = decl ? declLine(def.signature) : defLine(def.signature);
        if (rubyOwns(def.owner, owner) && kind)
            return def;
    }
    return {};
}

QString parentName(const QString &root, const QString &name)
{
    langEnsure(root, name);
    for (const LangDef &def : langHits(root, name, Lang::Ruby)) {
        const QString parent = rubyParentName(def.signature);
        if (!parent.isEmpty())
            return parent;
    }
    return {};
}

LangDef ancestorMethod(const QString &root, const QString &host, const QString &word)
{
    QString name = host;
    for (int step = 0; step < 6 && !name.isEmpty(); ++step) {
        const LangDef def = ownedKind(root, name, word, false);
        if (def.found())
            return def;
        const QString parent = parentName(root, name);
        if (parent == name)
            return {};
        name = parent;
    }
    return {};
}

void builtFrom(const QString &root, const QString &host, const QString &word, int depth, QSet<QString> *seen,
               QStringList *types)
{
    if (depth > 2 || word.isEmpty() || seen->contains(word))
        return;
    seen->insert(word);
    const LangDef def = ancestorMethod(root, host, word);
    if (!def.found())
        return;
    QStringList calls;
    rubyNoteBuilt(def.signature + QLatin1Char('\n') + def.body, types, &calls);
    for (const QString &call : calls)
        builtFrom(root, host, call, depth + 1, seen, types);
}

bool instanceDef(const LangDef &def)
{
    return !def.signature.contains(QLatin1String("def self."));
}

void keepHit(const LangDef &def, QList<LangDef> *found, QList<LangDef> *plain)
{
    found->append(def);
    if (instanceDef(def))
        plain->append(def);
}

QList<LangDef> typedHits(const QList<LangDef> &hits, const QStringList &types)
{
    QList<LangDef> found;
    QList<LangDef> plain;
    for (const QString &type : types) {
        for (const LangDef &def : hits) {
            if (rubyOwns(def.owner, type))
                keepHit(def, &found, &plain);
        }
    }
    return plain.isEmpty() ? found : plain;
}

QString headWord(const QString &signature)
{
    const QString line = signature.trimmed();
    return line.left(wordEnd(line, 0));
}

LangDef chainPick(const QList<LangDef> &hits, const LangSpot &spot, const QString &host)
{
    const LangDef decl = ownedKind(spot.root, host, spot.receiver.mid(host.size() + 1), true);
    if (!decl.found())
        return {};
    QStringList types;
    QSet<QString> seen;
    builtFrom(spot.root, host, headWord(decl.signature), 0, &seen, &types);
    return langPick(typedHits(hits, types), spot.path);
}

} // namespace

// Class.metric.method opens the method on the class that declaration builds.
LangDef rubyChain(const QList<LangDef> &hits, const LangSpot &spot)
{
    const int dot = spot.receiver.lastIndexOf(QLatin1Char('.'));
    const QString host = dot > 0 ? spot.receiver.left(dot) : QString();
    if (host.isEmpty() || !host.at(0).isUpper())
        return {};
    return chainPick(hits, spot, host);
}
