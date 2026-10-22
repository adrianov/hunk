// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "LangTune.hpp"

namespace {

QString finderType(const QString &text)
{
    QString found;
    int from = 0;
    while (from < text.size()) {
        const int dot = text.indexOf(QLatin1Char('.'), from);
        if (dot < 0)
            return found;
        if (finderWord(wordAt(text, rubySkip(text, dot + 1))) && !keepOwner(&found, constBefore(text, dot)))
            return {};
        from = dot + 1;
    }
    return found;
}

QString lineConst(const QString &line)
{
    const QString text = line.trimmed();
    if (text.isEmpty() || text == QLatin1String("end"))
        return {};
    return rubyConst(text, 0, rubyWordEnd(text, 0));
}

QString bodyConst(const QString &body)
{
    QString found;
    for (const QString &line : body.split(QLatin1Char('\n'))) {
        const QString type = lineConst(line);
        if (type.isEmpty())
            continue;
        if (!keepOwner(&found, type))
            return {};
    }
    return found;
}

QString optionType(const QString &text, int at)
{
    at = rubySkip(text, at);
    if (at >= text.size())
        return {};
    const QChar ch = text.at(at);
    if (ch == '\'' || ch == '"') {
        const int end = text.indexOf(ch, at + 1);
        return end <= at ? QString() : text.mid(at + 1, end - at - 1);
    }
    return rubyConst(text, at, rubyWordEnd(text, at));
}

QString classOption(const QString &text)
{
    const int at = text.indexOf(QLatin1String("class_name"));
    const int colon = text.indexOf(':', at);
    if (at < 0 || colon < 0)
        return {};
    return optionType(text, colon + 1);
}

QString camelName(const QString &word)
{
    QString out;
    bool upper = true;
    for (const QChar ch : word) {
        if (ch == '_') {
            upper = true;
            continue;
        }
        out += upper ? ch.toUpper() : ch.toLower();
        upper = false;
    }
    return out;
}

QString singular(const QString &name)
{
    if (name.endsWith(QLatin1String("ies")))
        return name.chopped(3) + QLatin1Char('y');
    for (const char *end : {"sses", "ches", "shes", "xes"}) {
        if (name.endsWith(QLatin1String(end)))
            return name.chopped(2);
    }
    return name.endsWith(QLatin1Char('s')) ? name.chopped(1) : name;
}

// belongs_to :market is Market, has_many :exchanger_triggers is ExchangerTrigger, unless class_name says otherwise.
QString assocType(const QString &signature, const QString &name)
{
    const QString line = signature.trimmed();
    const bool many = line.startsWith(QLatin1String("has_many"));
    if (!many && !line.startsWith(QLatin1String("belongs_to")) && !line.startsWith(QLatin1String("has_one")))
        return {};
    const QString named = classOption(signature);
    return named.isEmpty() ? camelName(many ? singular(name) : name) : named;
}

bool specFile(const QString &path)
{
    return path.startsWith(QLatin1String("spec/")) || path.startsWith(QLatin1String("test/"))
        || path.contains(QLatin1String("/spec/")) || path.contains(QLatin1String("/test/"));
}

QString defType(const LangDef &def, const QString &name)
{
    const QString assoc = assocType(def.signature, name);
    if (!assoc.isEmpty())
        return assoc;
    if (!def.signature.trimmed().startsWith(QLatin1String("def ")))
        return {};
    const QString body = bodyConst(def.body);
    return body.isEmpty() ? finderType(def.signature + QLatin1Char('\n') + def.body) : body;
}

QString agreed(const QList<LangDef> &hits, const QString &name)
{
    QString found;
    for (const LangDef &def : hits) {
        if (specFile(def.path))
            continue;
        if (!keepOwner(&found, defType(def, name)))
            return {};
    }
    return found;
}

} // namespace

bool finderWord(const QString &word)
{
    return word == QLatin1String("new") || word == QLatin1String("find") || word == QLatin1String("find_by")
        || word == QLatin1String("find_by!") || word == QLatin1String("create") || word == QLatin1String("create!");
}

QString wordAt(const QString &text, int at)
{
    QString word;
    int end = at;
    rubyWord(text, at, &word, &end);
    return end > at && word.size() == end - at ? word : QString();
}

QString methodType(const QString &root, const QString &name)
{
    langEnsure(root, name);
    return agreed(langHits(root, name, Lang::Ruby), name);
}
