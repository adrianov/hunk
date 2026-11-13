// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QRegularExpression>

namespace {

// Visibility and callbacks such as before_action :name point at a method; they do not declare one.
bool callWord(const QString &word)
{
    static const QRegularExpression words(QStringLiteral(
        "^(((skip|prepend|append)_)?(before|after|around)_\\w+|validate|helper_method|module_function"
        "|private|protected|public|(private|public)_(constant|class_method))$"));
    return words.match(word).hasMatch();
}

bool symbolArg(const QString &line, int at, QString *name)
{
    if (at + 1 >= line.size() || line.at(at) != QLatin1Char(':') || line.at(at + 1) == QLatin1Char(':'))
        return false;
    return rubyWord(line, at + 1, name, &at);
}

bool quoteArg(const QString &line, int at, QString *name)
{
    if (at >= line.size() || (line.at(at) != QLatin1Char('"') && line.at(at) != QLatin1Char('\'')))
        return false;
    const int end = line.indexOf(line.at(at), at + 1);
    if (end <= at + 1)
        return false;
    *name = line.mid(at + 1, end - at - 1);
    return true;
}

bool keywordArg(const QString &line, int at, QString *name)
{
    int next = 0;
    if (!rubyWord(line, at, name, &next) || name->at(0).isUpper())
        return false;
    next = rubySkip(line, next);
    return next < line.size() && line.at(next) == QLatin1Char(':')
        && (next + 1 >= line.size() || line.at(next + 1) != QLatin1Char(':'));
}

bool dslArg(const QString &line, int at, QString *name)
{
    at = rubySkip(line, at);
    if (at < line.size() && line.at(at) == QLatin1Char('('))
        at = rubySkip(line, at + 1);
    return symbolArg(line, at, name) || quoteArg(line, at, name) || keywordArg(line, at, name);
}

} // namespace

bool rubyDsl(const QString &line, QString *name)
{
    int next = 0;
    QString word;
    if (!rubyWord(line, rubySkip(line, 0), &word, &next) || callWord(word))
        return false;
    return dslArg(line, next, name);
}
