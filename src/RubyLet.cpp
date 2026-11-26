// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

// RSpec let / let! opens that helper from a bare use such as older_trade.

#include "LangTune.hpp"
#include "RubyDetail.hpp"

namespace {

LangDef noteLet(const QStringList &lines, int line, const QString &path)
{
    LangDef def = rubyAt(lines, line, path);
    if (!rubyBlockOpen(def.signature))
        def.body.clear();
    return def;
}

LangDef nearLet(const QList<LangDef> &lets, const LangSpot &spot)
{
    LangDef best;
    for (const LangDef &def : lets) {
        if (def.path != spot.path || def.line > spot.line)
            continue;
        if (!best.found() || def.line > best.line)
            best = def;
    }
    return best;
}

QList<LangDef> letHits(const QString &root, const QString &name)
{
    QList<LangDef> lets;
    for (const LangDef &def : langHits(root, name, Lang::Ruby)) {
        if (rubyLetStart(def.signature))
            lets.append(def);
    }
    return lets;
}

bool letWord(const QString &word)
{
    return word == QLatin1String("let") || word == QLatin1String("let!");
}

} // namespace

bool rubyLetStart(const QString &line)
{
    QString word;
    int next = 0;
    RubyArg group;
    if (!rubyHead(line, &word, &next) || next <= 0 || !letWord(word))
        return false;
    return rubyTake(line, next, &group) == RubyKind::Found;
}

bool rubyLet(const QString &text, QString *name)
{
    QString word;
    int next = 0;
    RubyArg group;
    if (!rubyHead(text, &word, &next) || next <= 0 || !letWord(word))
        return false;
    if (rubyTake(text, next, &group) != RubyKind::Found)
        return false;
    *name = group.name;
    return true;
}

void noteLetGroups(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    for (int line = 1; line <= lines.size(); ++line) {
        QString name;
        if (!rubyLetStart(lines.at(line - 1)))
            continue;
        if (rubyLet(rubyJoin(lines, line, rubySigEnd(lines, line)), &name))
            (*out)[name].push_back(noteLet(lines, line, path));
    }
}

bool rubyLetExtra(const LangSpot &spot, LangDef *out)
{
    if (!spot.receiver.isEmpty() || spot.name.isEmpty())
        return false;
    langEnsure(spot.root, spot.name);
    const QList<LangDef> lets = letHits(spot.root, spot.name);
    if (lets.isEmpty())
        return false;
    const LangDef near = nearLet(lets, spot);
    if (near.found()) {
        if (near.line == spot.line) {
            *out = {};
            return true;
        }
        *out = near;
        return true;
    }
    if (!rubySpecPath(spot.path))
        return false;
    *out = langPick(lets, spot.path);
    return out->found();
}
