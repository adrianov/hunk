// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

int skipOp(const QString &line, int at)
{
    while (at < line.size() && QStringLiteral("+-*/%&|^").contains(line.at(at)))
        ++at;
    if (at + 1 < line.size() && QStringLiteral("=>~").contains(line.at(at + 1)))
        return -1;
    return at;
}

int rhsFrom(const QString &line, int end)
{
    const int at = skipOp(line, rubySkip(line, end));
    if (at < 0 || at >= line.size() || line.at(at) != QLatin1Char('='))
        return -1;
    return at + 1;
}

QString assignExpr(const QString &line, const QString &name)
{
    const int at = line.indexOf(name);
    if (at < 0 || !rubyWordHit(line, at, name))
        return {};
    const int rhs = rhsFrom(line, at + name.size());
    return rhs < 0 ? QString() : line.mid(rhs).trimmed();
}

QString lastExpr(const QStringList &lines, int from, int to, const QString &name)
{
    QString expr;
    for (int at = from; at <= to; ++at) {
        const QString rhs = assignExpr(lines.at(at - 1), name);
        if (!rhs.isEmpty())
            expr = rhs;
    }
    return expr;
}

QString constCall(const QString &expr)
{
    const int at = rubySkip(expr, 0);
    const QString type = rubyConst(expr, at, rubyWordEnd(expr, at));
    const int dot = at + type.size();
    if (type.isEmpty() || dot >= expr.size() || expr.at(dot) != '.')
        return {};
    return finderWord(wordAt(expr, rubySkip(expr, dot + 1))) ? type : QString();
}

QString tailCall(const QString &expr)
{
    const int dot = expr.lastIndexOf(QLatin1Char('.'));
    if (dot < 0)
        return {};
    const QString word = wordAt(expr, dot + 1);
    return word.isEmpty() || word.at(0).isUpper() ? QString() : word;
}

QString exprType(const QString &root, const QString &expr)
{
    const int at = rubySkip(expr, 0);
    if (at < expr.size() && expr.at(at).isUpper())
        return constCall(expr);
    const QString word = tailCall(expr);
    return word.isEmpty() ? QString() : methodType(root, word);
}

QString localType(const LangSpot &spot, const RubyBound &bound)
{
    if (bound.opened <= 0)
        return {};
    const QString expr = lastExpr(bound.lines, bound.opened, spot.line, spot.receiver);
    return expr.isEmpty() ? QString() : exprType(spot.root, expr);
}

} // namespace

// A local assigned from a method opens that method on the class it returns.
QString rubyTyped(const LangSpot &spot, const QString &line)
{
    const QString lead = rubyLead(spot, line);
    if (!lead.isEmpty())
        return lead;
    if (!plainName(spot.receiver))
        return {};
    const RubyBound bound = rubyBound(spot);
    const QString local = localType(spot, bound);
    return local.isEmpty() ? rubyFollow(spot, bound) : local;
}
