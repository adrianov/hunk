// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangTune.hpp"

const LangTune *rubyLayer();

namespace {

const LangTune *const *layers()
{
    static const LangTune *list[] = {rubyLayer(), nullptr};
    return list;
}

} // namespace

const LangTune *langTune(Lang lang)
{
    for (const LangTune *const *item = layers(); *item; ++item) {
        if ((*item)->lang == lang)
            return *item;
    }
    return nullptr;
}

bool tuneKind(const LangTune *tune, SynKind kind)
{
    if (kind == SynKind::Method)
        return tune->method;
    if (kind == SynKind::Variable)
        return tune->variable;
    if (kind == SynKind::Type)
        return tune->type;
    return kind == SynKind::Keyword && tune->keyword;
}

QString langSeek(const QString &root, const QString &path, int line, const QString &name)
{
    const LangTune *tune = langTune(langFor(path));
    if (tune && tune->seek)
        return tune->seek(root, path, line, name);
    return name;
}

void langReset(int ticket)
{
    for (const LangTune *const *item = layers(); *item; ++item) {
        if ((*item)->reset)
            (*item)->reset(ticket);
    }
}
