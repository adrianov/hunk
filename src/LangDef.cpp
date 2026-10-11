// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"

#include "RubyDef.hpp"
#include "SyntaxRule.hpp"

void warmLang(const QString &root, QPointer<QObject> watch, std::function<void()> done)
{
    rubyWarm(root, watch, std::move(done));
}

bool langReady(const QString &root, const QString &path)
{
    if (langFor(path) != Lang::Ruby)
        return true;
    return rubyReady(root);
}

LangDef lookupDef(const LangSpot &spot)
{
    if (spot.name.isEmpty() || spot.root.isEmpty())
        return {};
    // Add a language here. It should return the same path, comment, signature, and body.
    if (langFor(spot.path) == Lang::Ruby)
        return rubyLookup(spot);
    return {};
}
