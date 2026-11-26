// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

// Grape use :group opens params :group. Params index and helpers live beside this file.

#include "LangTune.hpp"
#include "RubyDef.hpp"
#include "RubyDetail.hpp"

namespace {

bool readSym(const QString &text, int at, RubyArg *group)
{
    if (at + 1 >= text.size() || text.at(at) != QLatin1Char(':') || text.at(at + 1) == QLatin1Char(':'))
        return false;
    int next = 0;
    if (!rubyWord(text, at + 1, &group->name, &next))
        return false;
    group->start = at;
    group->end = next;
    return true;
}

bool grapeUse(const QStringList &lines, int call, int at, RubyArg *first)
{
    if (!rubyReadFirst(lines, call, at, first))
        return false;
    const QString &text = lines.at(first->line - 1);
    return first->start < text.size() && text.at(first->start) == QLatin1Char(':');
}

void takeSyms(const QString &text, int at, int line, QList<RubyArg> *syms)
{
    RubyArg sym;
    if (!readSym(text, at, &sym))
        return;
    sym.line = line;
    syms->append(sym);
    at = sym.end;
    while (true) {
        at = rubySkip(text, at);
        if (at >= text.size() || text.at(at) != QLatin1Char(','))
            break;
        at = rubySkip(text, at + 1);
        if (!readSym(text, at, &sym))
            break;
        sym.line = line;
        syms->append(sym);
        at = sym.end;
    }
}

bool fillUse(const QStringList &lines, int at, RubyArg *group, QList<RubyArg> *syms)
{
    int next = 0;
    if (!rubyHead(lines.at(at - 1), &group->word, &next) || next <= 0 || !rubyUseWord(group->word))
        return false;
    RubyArg first;
    if (!grapeUse(lines, at, next, &first))
        return false;
    *group = first;
    group->word = QLatin1String("use");
    group->call = at;
    takeSyms(lines.at(first.line - 1), first.start, first.line, syms);
    return !syms->isEmpty();
}

bool loadUse(const QStringList &lines, int line, RubyArg *group, QList<RubyArg> *syms)
{
    for (int at = line; at >= 1 && line - at < kRubyArgLook; --at) {
        if (!fillUse(lines, at, group, syms))
            continue;
        return rubyArgOwns(*group, line, at);
    }
    return false;
}

} // namespace

bool rubyUseWord(const QString &word)
{
    return word == QLatin1String("use");
}

QString rubyUseKey(const LangSpot &spot, const QStringList &lines)
{
    RubyArg group;
    QList<RubyArg> syms;
    if (!loadUse(lines, spot.line, &group, &syms))
        return {};
    if (spot.line == group.call && spot.name == group.word
        && rubyOnWord(lines.at(spot.line - 1), spot.column, group.word))
        return syms.constFirst().name;
    return rubySymAt(syms, spot.line, spot.column, spot.name);
}

QString rubyUseArgName(const QStringList &lines, int line, int column)
{
    RubyArg group;
    QList<RubyArg> syms;
    if (!loadUse(lines, line, &group, &syms))
        return {};
    return rubySymAt(syms, line, column);
}

QString rubyUseSought(const QStringList &lines, int line)
{
    RubyArg group;
    QList<RubyArg> syms;
    if (!fillUse(lines, line, &group, &syms))
        return {};
    return syms.constFirst().name;
}

QString rubyGrapeSeek(const QString &root, const QString &path, int line, const QString &name)
{
    const QString helpers = rubyHelpersSeek(root, path, line, name);
    return helpers.isEmpty() ? rubyGroupSeek(root, path, line, name) : helpers;
}

bool rubyGrapeExtra(const LangSpot &spot, LangDef *out)
{
    return rubyHelpersExtra(spot, out) || rubyGroupExtra(spot, out) || rubyLetExtra(spot, out)
        || rubyExtra(spot, out);
}
