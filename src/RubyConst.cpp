// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool constChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

int skipConst(const QString &text, int at)
{
    while (at < text.size() && constChar(text.at(at)))
        ++at;
    return at;
}

bool scopeAt(const QString &text, int at)
{
    return at >= 0 && at + 1 < text.size() && text.at(at) == QLatin1Char(':') && text.at(at + 1) == QLatin1Char(':');
}

int constLeft(const QString &text, int start)
{
    int left = start;
    while (scopeAt(text, left - 2)) {
        int at = left - 2;
        while (at > 0 && constChar(text.at(at - 1)))
            --at;
        left = at;
    }
    return left;
}

int constRight(const QString &text, int end)
{
    int right = end;
    while (scopeAt(text, right)) {
        if (right + 2 >= text.size() || !text.at(right + 2).isUpper())
            break;
        right = skipConst(text, right + 2);
    }
    return right;
}

int bareConst(const QString &text, int start)
{
    if (start + 1 < text.size() && text.at(start) == QLatin1Char(':') && text.at(start + 1).isUpper())
        return start + 1;
    return start;
}

bool assignMark(const QString &line, int at)
{
    if (at >= line.size() || line.at(at) != QLatin1Char('='))
        return false;
    if (at + 1 >= line.size())
        return true;
    const QChar next = line.at(at + 1);
    return next != QLatin1Char('=') && next != QLatin1Char('>') && next != QLatin1Char('~');
}

bool keyAt(const QString &line, QLatin1String key, int *at)
{
    const int cursor = rubySkip(line, 0);
    if (!line.mid(cursor).startsWith(key))
        return false;
    const int word = rubySkip(line, cursor + int(key.size()));
    if (word < line.size() && line.at(word).isUpper() && word > cursor + int(key.size())) {
        *at = word;
        return true;
    }
    return false;
}

} // namespace

QString rubyConst(const QString &text, int start, int end)
{
    start = bareConst(text, start);
    if (start >= text.size() || !text.at(start).isUpper())
        return {};
    const int left = constLeft(text, start);
    return text.mid(left, constRight(text, end) - left);
}

bool rubyTypeName(const QString &line, QString *name)
{
    int at = 0;
    if (!keyAt(line, QLatin1String("class"), &at) && !keyAt(line, QLatin1String("module"), &at))
        return false;
    const QString path = rubyConst(line, at, skipConst(line, at));
    if (path.isEmpty())
        return false;
    *name = path;
    return true;
}

bool rubyAssign(const QString &line, QString *name)
{
    const int at = rubySkip(line, 0);
    if (at >= line.size() || !line.at(at).isUpper())
        return false;
    const QString path = rubyConst(line, at, skipConst(line, at));
    if (path.isEmpty() || !assignMark(line, rubySkip(line, at + path.size())))
        return false;
    *name = path;
    return true;
}

void rubyConstDef(const QString &owner, const QString &name, QHash<QString, QList<LangDef>> *out)
{
    (*out)[name].last().body.clear();
    for (QString scope = owner; !scope.isEmpty(); scope = scope.section(QStringLiteral("::"), 1))
        (*out)[scope + QStringLiteral("::") + name].append((*out)[name].constLast());
}
