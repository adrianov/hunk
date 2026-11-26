// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"
#include "RubyDef.hpp"

namespace {

constexpr int kLook = 8;

bool readFirst(const QStringList &lines, int call, int at, RubyArg *group)
{
    for (int line = call; line <= lines.size() && line < call + kLook; ++line) {
        const RubyKind kind = rubyTake(lines.at(line - 1), line == call ? at : 0, group);
        if (kind == RubyKind::Wait)
            continue;
        if (kind != RubyKind::Found)
            return false;
        group->line = line;
        return true;
    }
    return false;
}

bool fillGroup(const QStringList &lines, int at, RubyArg *group)
{
    int next = 0;
    if (!rubyHead(lines.at(at - 1), &group->word, &next) || next <= 0 || !rubyIncludeWord(group->word))
        return false;
    group->call = at;
    return readFirst(lines, at, next, group);
}

bool owns(const RubyArg &group, int line, int at)
{
    return group.line == line || at == line;
}

RubyArg argAt(const QStringList &lines, int line)
{
    RubyArg group;
    for (int at = line; at >= 1 && line - at < kLook; --at) {
        if (!fillGroup(lines, at, &group))
            continue;
        if (owns(group, line, at))
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

QString groupKey(const LangSpot &spot, const QStringList &lines)
{
    const RubyArg group = argAt(lines, spot.line);
    if (group.name.isEmpty())
        return {};
    if (onCall(group, spot, lines.at(spot.line - 1)) || onArg(group, spot))
        return group.name;
    return {};
}

QList<LangDef> codeHits(const QList<LangDef> &hits)
{
    QList<LangDef> code;
    for (const LangDef &def : hits) {
        if (!rubySharedStart(def.signature))
            code.append(def);
    }
    return code;
}

LangDef groupFind(const QString &root, const QString &key, const QString &path)
{
    langEnsure(root, key);
    QList<LangDef> groups;
    for (const LangDef &def : langHits(root, key, Lang::Ruby)) {
        if (rubySharedStart(def.signature))
            groups.append(def);
    }
    return langPick(groups, path);
}

void noteOne(const QStringList &lines, int line, const QString &path, const QString &name,
             QHash<QString, QList<LangDef>> *out)
{
    (*out)[name].push_back(rubyAt(lines, line, path));
}

void noteShared(const QStringList &lines, int line, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    QString name;
    if (!rubySharedStart(lines.at(line - 1)))
        return;
    if (rubyShared(rubyJoin(lines, line, rubySigEnd(lines, line)), &name))
        noteOne(lines, line, path, name, out);
}

void noteGroups(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    for (int line = 1; line <= lines.size(); ++line)
        noteShared(lines, line, path, out);
}

QString soughtName(const QStringList &lines, int line, const QString &word)
{
    int next = 0;
    QString call;
    RubyArg group;
    if (line < 1 || line > lines.size() || !rubyHead(lines.at(line - 1), &call, &next) || next <= 0 || call != word)
        return {};
    return readFirst(lines, line, next, &group) ? group.name : QString();
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
    const RubyArg group = lines.isEmpty() ? RubyArg() : argAt(lines, line);
    if (group.line != line || column < group.start || column >= group.end)
        return {};
    return group.name;
}

QString rubyGroupSeek(const QString &root, const QString &path, int line, const QString &name)
{
    const QStringList lines = rubyFileLines(root, path, line);
    if (!rubyIncludeWord(name) || lines.isEmpty())
        return rubySeek(root, path, line, name);
    const QString key = soughtName(lines, line, name);
    return key.isEmpty() ? rubySeek(root, path, line, name) : key;
}

bool rubyGroupExtra(const LangSpot &spot, LangDef *out)
{
    const QStringList lines = rubyFileLines(spot.root, spot.path, spot.line);
    const QString key = lines.isEmpty() || spot.name.isEmpty() ? QString() : groupKey(spot, lines);
    if (key.isEmpty())
        return rubyExtra(spot, out);
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
    noteGroups(lines, path, out);
}
