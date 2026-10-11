// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

QString nestEnd(const QString &open)
{
    int at = 0;
    while (at < open.size() && open.at(at).isSpace())
        ++at;
    return open.left(at) + QStringLiteral("end");
}

QString nestEnds(const QStringList &opens)
{
    QStringList tails;
    for (const QString &open : opens)
        tails.prepend(nestEnd(open));
    return tails.join(QLatin1Char('\n'));
}

} // namespace

void rubyWrap(LangDef *def, const QStringList &opens)
{
    if (opens.isEmpty())
        return;
    QString head = opens.join(QLatin1Char('\n'));
    if (!def->comment.isEmpty())
        head += QLatin1Char('\n') + def->comment;
    def->comment.clear();
    def->signature.prepend(head + QLatin1Char('\n'));
    const QString ends = nestEnds(opens);
    if (def->body.isEmpty())
        def->body = ends;
    else
        def->body += QLatin1Char('\n') + ends;
}
