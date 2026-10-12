// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool wordEdge(const QString &text, int at)
{
    return at == 0 || !rubyWordChar(text.at(at - 1));
}

int skipString(const QString &text, int at)
{
    const QChar quote = text.at(at);
    for (++at; at < text.size(); ++at) {
        if (text.at(at) == QLatin1Char('\\'))
            ++at;
        else if (text.at(at) == quote)
            return at + 1;
    }
    return text.size();
}

bool identAt(const QString &text, int at)
{
    return at < text.size() && rubyWordChar(text.at(at));
}

bool callNew(const QString &text, int at)
{
    return text.mid(at).startsWith(QLatin1String(".new")) && !identAt(text, at + 4);
}

bool moreConst(const QString &text, int end)
{
    return end + 2 < text.size() && text.at(end) == QLatin1Char(':') && text.at(end + 1) == QLatin1Char(':')
        && text.at(end + 2).isUpper();
}

int readConst(const QString &text, int at, QString *path)
{
    int end = rubyWordEnd(text, at);
    *path = text.mid(at, end - at);
    while (moreConst(text, end)) {
        const int next = rubyWordEnd(text, end + 2);
        path->append(text.mid(end, next - end));
        end = next;
    }
    return end;
}

void keepType(QStringList *types, const QString &path)
{
    if (!types->contains(path))
        types->append(path);
}

int takeConst(const QString &text, int at, QStringList *types)
{
    QString path;
    const int end = readConst(text, at, &path);
    if (callNew(text, end))
        keepType(types, path);
    return end;
}

bool bareCall(const QString &text, int at, int end)
{
    if (at > 0 && text.at(at - 1) == QLatin1Char('.'))
        return false;
    const int next = rubySkip(text, end);
    return next < text.size() && text.at(next) == QLatin1Char('(');
}

void keepCall(QStringList *calls, const QString &word)
{
    if (!calls->contains(word))
        calls->append(word);
}

int takeCall(const QString &text, int at, QStringList *calls)
{
    const int end = rubyWordEnd(text, at);
    if (bareCall(text, at, end))
        keepCall(calls, text.mid(at, end - at));
    return end;
}

int skipHash(const QString &text, int at)
{
    const int nl = text.indexOf(QLatin1Char('\n'), at);
    return nl < 0 ? text.size() : nl + 1;
}

int skipMark(const QString &text, int at)
{
    const QChar ch = text.at(at);
    if (ch == QLatin1Char('"') || ch == QLatin1Char('\''))
        return skipString(text, at);
    return ch == QLatin1Char('#') ? skipHash(text, at) : at;
}

int takeName(const QString &text, int at, QStringList *types, QStringList *calls)
{
    if (!text.at(at).isLetter() || !wordEdge(text, at))
        return at + 1;
    return text.at(at).isUpper() ? takeConst(text, at, types) : takeCall(text, at, calls);
}

int takeToken(const QString &text, int at, QStringList *types, QStringList *calls)
{
    const int marked = skipMark(text, at);
    return marked == at ? takeName(text, at, types, calls) : marked;
}

} // namespace

void rubyNoteBuilt(const QString &text, QStringList *types, QStringList *calls)
{
    for (int at = 0; at < text.size();)
        at = takeToken(text, at, types, calls);
}
