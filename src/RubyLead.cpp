// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

namespace {

int wordLeft(const QString &text, int at)
{
    while (at > 0 && rubyWordChar(text.at(at - 1)))
        --at;
    return at;
}

bool scopeMark(const QString &text, int at)
{
    return at >= 2 && text.at(at - 1) == QLatin1Char(':') && text.at(at - 2) == QLatin1Char(':');
}

int chainLeft(const QString &text, int left)
{
    while (left > 0) {
        const int word = wordLeft(text, left);
        if (word == left)
            break;
        left = word;
        if (scopeMark(text, left))
            left -= 2;
        else if (left > 0 && text.at(left - 1) == QLatin1Char('.'))
            --left;
        else
            break;
    }
    return left;
}

QString callChain(const QString &text, int column, const QString &name)
{
    if (name.isEmpty() || column < 0)
        return {};
    const int at = wordLeft(text, qMin(column, text.size()));
    if (text.mid(at, name.size()) != name || at < 1 || text.at(at - 1) != QLatin1Char('.'))
        return {};
    const int left = chainLeft(text, at - 1);
    return text.mid(left, at - 1 - left);
}

QString chainHead(const QString &chain, QString *rest)
{
    const int dot = chain.indexOf(QLatin1Char('.'));
    if (dot <= 0)
        return {};
    *rest = chain.mid(dot + 1);
    return chain.left(dot);
}

bool plainAssign(const QString &text, int at, int depth)
{
    if (depth != 0 || at >= text.size() || text.at(at) != QLatin1Char('='))
        return false;
    return at + 1 >= text.size() || !QStringLiteral("=>~").contains(text.at(at + 1));
}

int nextDepth(int depth, QChar ch)
{
    if (ch == QLatin1Char('('))
        return depth + 1;
    return ch == QLatin1Char(')') && depth > 0 ? depth - 1 : depth;
}

QString assignConst(const QString &text)
{
    int depth = 0;
    for (int at = 0; at < text.size(); ++at) {
        if (plainAssign(text, at, depth)) {
            const int expr = rubySkip(text, at + 1);
            return rubyConst(text, expr, rubyWordEnd(text, expr));
        }
        depth = nextDepth(depth, text.at(at));
    }
    return {};
}

QString bodyConst(const QString &body)
{
    QString found;
    for (const QString &line : body.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed == QLatin1String("end"))
            continue;
        if (!found.isEmpty())
            return {};
        found = rubyConst(trimmed, 0, rubyWordEnd(trimmed, 0));
        if (found.isEmpty())
            return {};
    }
    return found;
}

bool methodDef(const LangDef &def, const QString &path)
{
    return def.path == path && def.signature.trimmed().startsWith(QLatin1String("def "));
}

QString methodConst(const LangSpot &spot, const QString &name)
{
    langEnsure(spot.root, name);
    QString found;
    for (const LangDef &def : langHits(spot.root, name, Lang::Ruby)) {
        if (!methodDef(def, spot.path))
            continue;
        const QString type = assignConst(def.signature);
        const QString body = type.isEmpty() ? bodyConst(def.body) : type;
        if (body.isEmpty())
            continue;
        if (!found.isEmpty())
            return {};
        found = body;
    }
    return found;
}

QString shownLine(const LangSpot &spot, const QString &line)
{
    return line.isEmpty() ? lineAt(spot.root, spot.path, spot.line) : line;
}

} // namespace

// def metrics = SomeClass lets metrics.metric.measure open that class's chain.
QString rubyLead(const LangSpot &spot, const QString &line)
{
    if (spot.line <= 0)
        return {};
    QString rest;
    const QString head = chainHead(callChain(shownLine(spot, line), spot.column, spot.name), &rest);
    if (head.isEmpty() || rest.isEmpty() || head.at(0).isUpper() || head.contains(QLatin1Char(':')))
        return {};
    const QString type = methodConst(spot, head);
    return type.isEmpty() ? QString() : type + QLatin1Char('.') + rest;
}
