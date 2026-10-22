// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

#include <algorithm>

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

// Words called after each dot outside brackets: exchanger_triggers and active in user.exchanger_triggers.active.
QStringList chainCalls(const QString &expr)
{
    QStringList calls;
    int depth = 0;
    for (int at = 0; at < expr.size(); ++at) {
        const QChar ch = expr.at(at);
        if (QStringLiteral("([{").contains(ch))
            ++depth;
        else if (QStringLiteral(")]}").contains(ch))
            --depth;
        else if (ch == QLatin1Char('.') && depth == 0)
            calls.append(wordAt(expr, at + 1));
    }
    return calls;
}

// A scope such as active, or a query such as where, returns the model of the call before it.
bool keepsModel(const QString &root, const QString &word)
{
    static const QStringList queries = {"all", "where", "not", "order", "joins", "includes", "merge", "limit", "lock"};
    if (queries.contains(word))
        return true;
    langEnsure(root, word);
    const QList<LangDef> hits = langHits(root, word, Lang::Ruby);
    return std::any_of(hits.cbegin(), hits.cend(),
                       [](const LangDef &def) { return def.signature.trimmed().startsWith(QLatin1String("scope ")); });
}

QString chainType(const QString &root, const QStringList &calls)
{
    for (auto call = calls.crbegin(); call != calls.crend(); ++call) {
        if (call->isEmpty() || call->at(0).isUpper())
            return {};
        const QString type = methodType(root, *call);
        if (!type.isEmpty() || !keepsModel(root, *call))
            return type;
    }
    return {};
}

QString exprType(const QString &root, const QString &expr)
{
    const int at = rubySkip(expr, 0);
    if (at < expr.size() && expr.at(at).isUpper())
        return constCall(expr);
    return chainType(root, chainCalls(expr));
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
