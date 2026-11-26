// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

#include <QRegularExpression>
#include <QSet>

namespace {

bool markEq(const QString &text, int at)
{
    if (text.at(at) != QLatin1Char('=') || (at + 1 < text.size() && text.at(at + 1) == QLatin1Char('=')))
        return false;
    return at == 0 || text.at(at - 1).isSpace() || text.at(at - 1) == QLatin1Char(')');
}

int eqAt(const QString &text)
{
    int depth = 0;
    for (int at = 0; at < text.size(); ++at) {
        const QChar ch = text.at(at);
        if (ch == QLatin1Char('('))
            ++depth;
        else if (ch == QLatin1Char(')') && depth > 0)
            --depth;
        else if (depth == 0 && markEq(text, at))
            return at;
    }
    return -1;
}

bool oneDef(const QString &signature)
{
    int count = 0;
    for (const QString &line : signature.split(QLatin1Char('\n'))) {
        if (line.trimmed().startsWith(QLatin1String("def ")))
            ++count;
    }
    return count == 1;
}

struct Hop {
    QString owner;
    QString name;
    bool instance = false;
};

bool fillHop(const QRegularExpressionMatch &match, const QString &owner, Hop *hop)
{
    hop->name = match.captured(1);
    hop->owner = owner;
    hop->instance = true;
    if (!hop->name.isEmpty())
        return true;
    hop->owner = match.captured(2);
    hop->name = match.captured(3);
    if (hop->name.isEmpty()) {
        hop->name = match.captured(4);
        hop->instance = false;
    }
    return !hop->owner.isEmpty() && !hop->name.isEmpty();
}

bool parseHop(const QString &rhs, const QString &owner, Hop *hop)
{
    static const QRegularExpression re(QStringLiteral(
        "^\\s*(?:new(?:\\([^)]*\\))?\\.(\\w+[!?]?)(?:\\([^)]*\\))?|([A-Z]\\w*(?:::[A-Z]\\w*)*)\\.(?:new(?:\\([^)]*\\))?\\.("
        "\\w+[!?]?)|(\\w+[!?]?))(?:\\([^)]*\\))?)\\s*(?:#[^\\n]*)?\\s*$"));
    const QRegularExpressionMatch match = re.match(rhs);
    return match.hasMatch() && fillHop(match, owner, hop);
}

bool wantDef(const LangDef &def, bool instance)
{
    return def.signature.trimmed().startsWith(QLatin1String("def "))
        && def.signature.contains(QLatin1String("def self.")) != instance;
}

LangDef ownedMethod(const QString &root, const Hop &hop)
{
    langEnsure(root, hop.name);
    QList<LangDef> hits;
    for (const LangDef &def : langHits(root, hop.name, Lang::Ruby)) {
        if (wantDef(def, hop.instance) && rubyOwns(def.owner, hop.owner))
            hits.append(def);
    }
    return hits.size() == 1 ? hits.constFirst() : LangDef();
}

LangDef nextDef(const QString &root, const LangDef &def)
{
    const int eq = oneDef(def.signature) ? eqAt(def.signature) : -1;
    Hop hop;
    if (eq < 0 || !parseHop(def.signature.mid(eq + 1), def.owner, &hop))
        return {};
    return ownedMethod(root, hop);
}

QString joinDef(const LangDef &next)
{
    QString text = next.signature;
    if (!next.body.isEmpty())
        text.append(QLatin1Char('\n')).append(next.body);
    if (!next.comment.isEmpty())
        text.prepend(QLatin1Char('\n')).prepend(next.comment);
    return text;
}

QString blockText(const LangDef &from, const LangDef &next)
{
    const QString code = joinDef(next);
    if (next.path == from.path)
        return code;
    return next.path + QLatin1Char(':') + QString::number(next.line) + QLatin1Char('\n') + code;
}

QString seenKey(const LangDef &def)
{
    return def.path + QLatin1Char(':') + QString::number(def.line);
}

} // namespace

LangDef rubyForward(const QString &root, const LangDef &def)
{
    if (!def.found())
        return def;
    LangDef shown = def;
    LangDef cursor = def;
    QSet<QString> seen;
    seen.insert(seenKey(def));
    for (int step = 0; step < 4; ++step) {
        cursor = nextDef(root, cursor);
        const QString key = seenKey(cursor);
        if (!cursor.found() || seen.contains(key))
            break;
        seen.insert(key);
        shown.body.append(QLatin1Char('\n')).append(blockText(def, cursor));
    }
    return shown;
}
