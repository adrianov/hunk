// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangTune.hpp"
#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>

namespace {

int indentOf(const QString &line)
{
    int col = 0;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' '))
            ++col;
        else if (ch == QLatin1Char('\t'))
            col += 4;
        else
            break;
    }
    return col;
}

struct Encl {
    QString method;
    QString owner;
    QString parent;
    int defLine = 0;
};

bool takeDef(const QString &text, int at, int *limit, Encl *out)
{
    QString name;
    if (!out->method.isEmpty() || !rubyDefName(text, &name))
        return false;
    out->method = name;
    out->defLine = at;
    *limit = indentOf(text);
    return true;
}

bool takeType(const QString &text, Encl *out)
{
    QString name;
    if (out->method.isEmpty() || !rubyTypeName(text, &name))
        return false;
    out->owner = name;
    out->parent = rubyParentName(text);
    return true;
}

bool enclose(const QStringList &lines, int line, Encl *out)
{
    if (line < 1 || line > lines.size())
        return false;
    int limit = indentOf(lines.at(line - 1));
    for (int at = line; at >= 1; --at) {
        const QString text = lines.at(at - 1);
        if (indentOf(text) > limit)
            continue;
        if (takeDef(text, at, &limit, out))
            continue;
        if (takeType(text, out))
            return true;
    }
    return !out->method.isEmpty();
}

QStringList fileLines(const QString &root, const QString &path)
{
    QFile file(QDir(root).filePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return rubyLines(QString::fromUtf8(file.readAll()));
}

} // namespace

QString rubyMethodAt(const QString &root, const QString &path, int line)
{
    Encl encl;
    enclose(fileLines(root, path), line, &encl);
    return encl.method;
}

LangDef rubySuper(const LangSpot &spot)
{
    Encl encl;
    if (!enclose(fileLines(spot.root, spot.path), spot.line, &encl) || encl.method.isEmpty())
        return {};
    langEnsure(spot.root, encl.method);
    const QList<LangDef> hits = langHits(spot.root, encl.method, Lang::Ruby);
    if (!encl.parent.isEmpty()) {
        const LangDef parent = ancestorDef(spot.root, encl.parent, hits, spot.path, encl.defLine);
        if (parent.found())
            return parent;
    }
    return joined(hostDefs(spot.root, encl.owner, hits, spot.path, encl.defLine));
}
