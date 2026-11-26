// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

// Grape helpers Module opens that module (not helpers do).

#include "LangTune.hpp"
#include "RubyDetail.hpp"

namespace {

bool helpersWord(const QString &line, int *at)
{
    const int cursor = rubySkip(line, 0);
    if (!line.mid(cursor).startsWith(QLatin1String("helpers")))
        return false;
    const int end = cursor + 7;
    if (end < line.size() && (line.at(end).isLetterOrNumber() || line.at(end) == QLatin1Char('_')))
        return false;
    *at = rubySkip(line, end);
    return true;
}

int moduleAt(const QString &line, int at)
{
    if (line.mid(at).startsWith(QLatin1String("::")))
        at += 2;
    at = rubySkip(line, at);
    return at < line.size() && line.at(at).isUpper() ? at : -1;
}

QString moduleName(const QString &line, int at)
{
    QString path = rubyConst(line, at, rubyWordEnd(line, at));
    while (path.startsWith(QLatin1String("::")))
        path.remove(0, 2);
    return path;
}

} // namespace

bool rubyHelpersMix(const QString &line, QString *name)
{
    int at = 0;
    if (!helpersWord(line, &at))
        return false;
    at = moduleAt(line, at);
    if (at < 0)
        return false;
    *name = moduleName(line, at);
    return !name->isEmpty();
}

QString rubyHelpersSeek(const QString &root, const QString &path, int line, const QString &name)
{
    if (name != QLatin1String("helpers"))
        return {};
    const QStringList lines = rubyFileLines(root, path, line);
    QString mod;
    if (lines.isEmpty() || !rubyHelpersMix(lines.at(line - 1), &mod))
        return {};
    return mod;
}

bool rubyHelpersExtra(const LangSpot &spot, LangDef *out)
{
    const QString mod = rubyHelpersSeek(spot.root, spot.path, spot.line, spot.name);
    if (mod.isEmpty())
        return false;
    langEnsure(spot.root, mod);
    *out = langPick(langHits(spot.root, mod, Lang::Ruby), spot.path);
    return true;
}
