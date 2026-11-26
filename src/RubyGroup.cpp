// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"
#include "RubyDef.hpp"

namespace {

bool fillInclude(const QStringList &lines, int at, RubyArg *group)
{
    int next = 0;
    if (!rubyHead(lines.at(at - 1), &group->word, &next) || next <= 0 || !rubyIncludeWord(group->word))
        return false;
    group->call = at;
    return rubyReadFirst(lines, at, next, group);
}

RubyArg includeAt(const QStringList &lines, int line)
{
    RubyArg group;
    for (int at = line; at >= 1 && line - at < kRubyArgLook; --at) {
        if (!fillInclude(lines, at, &group))
            continue;
        if (rubyArgOwns(group, line, at))
            return group;
        return {};
    }
    return {};
}

bool onCall(const RubyArg &group, const LangSpot &spot, const QString &line)
{
    return spot.line == group.call && spot.name == group.word && rubyOnWord(line, spot.column, group.word);
}

bool onArg(const RubyArg &group, const LangSpot &spot)
{
    return spot.line == group.line && spot.name == group.name && spot.column >= group.start
        && spot.column < group.end;
}

QString includeKey(const LangSpot &spot, const QStringList &lines)
{
    const RubyArg group = includeAt(lines, spot.line);
    if (group.name.isEmpty())
        return {};
    if (onCall(group, spot, lines.at(spot.line - 1)) || onArg(group, spot))
        return group.name;
    return {};
}

QString groupKey(const LangSpot &spot, const QStringList &lines)
{
    const QString include = includeKey(spot, lines);
    return include.isEmpty() ? rubyUseKey(spot, lines) : include;
}

// Shared context / examples and Grape params — not RSpec let.
bool groupStart(const QString &signature)
{
    return rubySharedStart(signature) || rubyParamsStart(signature);
}

QList<LangDef> codeHits(const QList<LangDef> &hits)
{
    QList<LangDef> code;
    for (const LangDef &def : hits) {
        if (!rubyNamedStart(def.signature))
            code.append(def);
    }
    return code;
}

LangDef groupFind(const QString &root, const QString &key, const QString &path)
{
    langEnsure(root, key);
    QList<LangDef> groups;
    for (const LangDef &def : langHits(root, key, Lang::Ruby)) {
        if (groupStart(def.signature))
            groups.append(def);
    }
    return langPick(groups, path);
}

void noteSharedGroups(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    for (int line = 1; line <= lines.size(); ++line) {
        QString name;
        if (!rubySharedStart(lines.at(line - 1)))
            continue;
        if (rubyShared(rubyJoin(lines, line, rubySigEnd(lines, line)), &name))
            (*out)[name].push_back(rubyAt(lines, line, path));
    }
}

QString soughtName(const QStringList &lines, int line, const QString &word)
{
    int next = 0;
    QString call;
    RubyArg group;
    if (line < 1 || line > lines.size() || !rubyHead(lines.at(line - 1), &call, &next) || next <= 0 || call != word)
        return {};
    if (rubyUseWord(word))
        return rubyUseSought(lines, line);
    return rubyReadFirst(lines, line, next, &group) ? group.name : QString();
}

} // namespace

bool rubySharedStart(const QString &line)
{
    QString word;
    int next = 0;
    if (!rubyHead(line, &word, &next) || next <= 0 || next > line.size())
        return false;
    return rubySharedWord(word);
}

bool rubyShared(const QString &text, QString *name)
{
    QString word;
    int next = 0;
    RubyArg group;
    if (!rubyHead(text, &word, &next) || next <= 0 || !rubySharedWord(word))
        return false;
    if (rubyTake(text, next, &group) != RubyKind::Found)
        return false;
    *name = group.name;
    return true;
}

QString rubyGroupName(const QString &root, const QString &path, int line, int column)
{
    const QStringList lines = rubyFileLines(root, path, line);
    if (lines.isEmpty())
        return {};
    const RubyArg include = includeAt(lines, line);
    if (include.line == line && column >= include.start && column < include.end)
        return include.name;
    return rubyUseArgName(lines, line, column);
}

QString rubyGroupSeek(const QString &root, const QString &path, int line, const QString &name)
{
    const QStringList lines = rubyFileLines(root, path, line);
    if (lines.isEmpty() || (!rubyIncludeWord(name) && !rubyUseWord(name)))
        return rubySeek(root, path, line, name);
    const QString key = soughtName(lines, line, name);
    return key.isEmpty() ? rubySeek(root, path, line, name) : key;
}

bool rubyGroupExtra(const LangSpot &spot, LangDef *out)
{
    const QStringList lines = rubyFileLines(spot.root, spot.path, spot.line);
    const QString key = lines.isEmpty() || spot.name.isEmpty() ? QString() : groupKey(spot, lines);
    if (key.isEmpty())
        return false;
    *out = groupFind(spot.root, key, spot.path);
    return true;
}

LangDef rubyGroupChoose(const QList<LangDef> &hits, const LangSpot &spot)
{
    return rubyChoose(codeHits(hits), spot);
}

void rubyGroupNote(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    noteRuby(lines, path, out);
    noteSharedGroups(lines, path, out);
    noteParamsGroups(lines, path, out);
    noteLetGroups(lines, path, out);
}
