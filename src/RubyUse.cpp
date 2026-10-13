// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool nameHit(const QString &line, int at, const QString &name)
{
    if (!rubyWordHit(line, at, name))
        return false;
    const int end = at + name.size();
    return end >= line.size() || (line.at(end) != QLatin1Char('?') && line.at(end) != QLatin1Char('!'));
}

bool nameAt(const QString &line, const QString &name)
{
    int from = 0;
    while (from < line.size()) {
        const int at = line.indexOf(name, from);
        if (at < 0)
            return false;
        if (nameHit(line, at, name))
            return true;
        from = at + qMax(1, name.size());
    }
    return false;
}

bool definedHere(const QString &line, const QString &name)
{
    QString found;
    if (rubyDefName(line, &found) && found == name)
        return true;
    if (rubyTypeName(line, &found) && found == name)
        return true;
    return rubyAssign(line, &found) && found == name;
}

bool dotted(const QString &line, int at)
{
    return at > 0 && line.at(at - 1) == QLatin1Char('.');
}

bool parenAfter(const QString &line, int end)
{
    return end < line.size() && line.at(end) == QLatin1Char('(');
}

bool callHere(const QString &line, const QString &name)
{
    int from = 0;
    while (from < line.size()) {
        const int at = line.indexOf(name, from);
        if (at < 0)
            return false;
        if (nameHit(line, at, name) && (dotted(line, at) || parenAfter(line, at + name.size())))
            return true;
        from = at + qMax(1, name.size());
    }
    return false;
}

bool keptUse(const QString &line, const QString &name)
{
    if (name.isEmpty() || definedHere(line, name) || !nameAt(line, name))
        return false;
    return name.at(0).isUpper() || callHere(line, name);
}

QString scopeText(const QStringList &lines)
{
    QStringList bits;
    for (const QString &line : lines) {
        if (line.contains(QStringLiteral("::")))
            bits.append(line);
    }
    return bits.join(QLatin1Char('\n'));
}

} // namespace

void rubyUses(const QStringList &lines, const QString &path, const QStringList &names, QHash<QString, QList<LangDef>> *out)
{
    const QString scopes = scopeText(lines);
    for (int line = 1; line <= lines.size(); ++line) {
        const QString text = lines.at(line - 1);
        if (text.trimmed().startsWith(QLatin1Char('#')))
            continue;
        for (const QString &name : names) {
            if (!keptUse(text, name))
                continue;
            LangDef use;
            use.path = path;
            use.line = line;
            use.signature = text;
            use.body = scopes;
            (*out)[name].append(use);
        }
    }
}
