// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDef.hpp"

#include "RubyDetail.hpp"

#include <QDir>

namespace {

LangDef inFile(const QString &text, const QString &path, const QString &name)
{
    if (text.isEmpty() || name.isEmpty())
        return {};
    const QStringList lines = rubyLines(text);
    for (int line = 1; line <= lines.size(); ++line) {
        QString got;
        if (rubyDefName(lines.at(line - 1), &got) && got == name)
            return rubyAt(lines, line, path);
    }
    return {};
}

bool definesHere(const QString &text, int line, const QString &name)
{
    const QStringList lines = rubyLines(text);
    if (line < 1 || line > lines.size() || name.isEmpty())
        return false;
    QString got;
    return rubyDefName(lines.at(line - 1), &got) && got == name;
}

} // namespace

LangDef rubyLookup(const LangSpot &spot)
{
    const QString text = rubyRead(QDir(spot.root).filePath(spot.path));
    if (definesHere(text, spot.line, spot.name))
        return {};
    const LangDef local = inFile(text, spot.path, spot.name);
    if (local.found() || !rubyReady(spot.root))
        return local;
    const RubyPlace place = rubyPick(rubyHits(spot.root, spot.name), spot.path);
    if (place.line <= 0 || place.path == spot.path)
        return {};
    return inFile(rubyRead(QDir(spot.root).filePath(place.path)), place.path, spot.name);
}
