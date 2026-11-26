// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

// Grape params :group declarations for use :group jumps.

#include "LangTune.hpp"
#include "RubyDetail.hpp"

bool rubyParamsWord(const QString &word)
{
    return word == QLatin1String("params");
}

bool rubyParamsStart(const QString &line)
{
    QString word;
    int next = 0;
    RubyArg group;
    if (!rubyHead(line, &word, &next) || next <= 0 || !rubyParamsWord(word))
        return false;
    return rubyTake(line, next, &group) == RubyKind::Found && line.at(group.start) == QLatin1Char(':');
}

bool rubyParams(const QString &text, QString *name)
{
    QString word;
    int next = 0;
    RubyArg group;
    if (!rubyHead(text, &word, &next) || next <= 0 || !rubyParamsWord(word))
        return false;
    if (rubyTake(text, next, &group) != RubyKind::Found || text.at(group.start) != QLatin1Char(':'))
        return false;
    *name = group.name;
    return true;
}

bool rubyNamedStart(const QString &signature)
{
    return rubySharedStart(signature) || rubyParamsStart(signature) || rubyLetStart(signature);
}

void noteParamsGroups(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    for (int line = 1; line <= lines.size(); ++line) {
        QString name;
        if (!rubyParamsStart(lines.at(line - 1)))
            continue;
        if (rubyParams(rubyJoin(lines, line, rubySigEnd(lines, line)), &name))
            (*out)[name].push_back(rubyAt(lines, line, path));
    }
}
