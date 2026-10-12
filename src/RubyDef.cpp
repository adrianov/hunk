// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDef.hpp"

#include "LangTune.hpp"
#include "RubyDetail.hpp"

namespace {

bool classReceiver(const QString &name)
{
    return !name.isEmpty() && name.at(0).isUpper();
}

} // namespace

bool rubyOwns(const QString &owner, const QString &name)
{
    if (owner.isEmpty() || name.isEmpty())
        return false;
    if (owner == name || owner.endsWith(QStringLiteral("::") + name))
        return true;
    return name.endsWith(QStringLiteral("::") + owner);
}

namespace {

QList<LangDef> ownedHits(const QList<LangDef> &hits, const QString &receiver)
{
    QList<LangDef> owned;
    for (const LangDef &def : hits) {
        if (rubyOwns(def.owner, receiver))
            owned.append(def);
    }
    return owned;
}

LangDef selfCall(const QList<LangDef> &defs, const QString &path)
{
    QList<LangDef> selves;
    for (const LangDef &def : defs) {
        if (def.signature.contains(QLatin1String("def self.")))
            selves.append(def);
    }
    return langPick(selves.isEmpty() ? defs : selves, path);
}

bool snakeBreak(const QString &word, int i)
{
    if (i == 0 || !word.at(i).isUpper())
        return false;
    const QChar prev = word.at(i - 1);
    if (prev.isLower() || prev.isDigit())
        return true;
    return i + 1 < word.size() && prev.isUpper() && word.at(i + 1).isLower();
}

QString snakeWord(const QString &word)
{
    QString out;
    for (int i = 0; i < word.size(); ++i) {
        if (snakeBreak(word, i))
            out += QLatin1Char('_');
        out += word.at(i).toLower();
    }
    return out;
}

QString modelFile(const QString &name)
{
    QString file;
    for (const QString &part : name.split(QStringLiteral("::"))) {
        if (part.isEmpty())
            continue;
        if (!file.isEmpty())
            file += QLatin1Char('/');
        file += snakeWord(part);
    }
    return file + QStringLiteral(".rb");
}

bool typeLine(const QString &signature)
{
    const QString line = signature.trimmed();
    return line.startsWith(QLatin1String("class ")) || line.startsWith(QLatin1String("module "));
}

bool fileFits(const QString &path, const QString &file)
{
    return path == file || path.endsWith(QLatin1Char('/') + file);
}

int modelScore(const LangDef &def, const QString &file)
{
    if (!typeLine(def.signature) || !fileFits(def.path, file))
        return 0;
    const QString canonical = QStringLiteral("app/models/") + file;
    int score = 2;
    if (def.path == canonical || def.path.endsWith(QLatin1Char('/') + canonical))
        score += 3;
    if (def.signature.contains(QLatin1Char('<')))
        ++score;
    return score;
}

QString camelWord(const QString &word)
{
    QString out;
    bool upper = true;
    for (const QChar ch : word) {
        if (ch == QLatin1Char('_')) {
            upper = true;
            continue;
        }
        out += upper ? ch.toUpper() : ch.toLower();
        upper = false;
    }
    return out;
}

QList<LangDef> looseHits(const QList<LangDef> &hits, const QString &receiver)
{
    const QString camel = camelWord(receiver);
    const QString file = snakeWord(camel) + QStringLiteral(".rb");
    QList<LangDef> owned;
    QList<LangDef> paths;
    for (const LangDef &def : hits) {
        if (rubyOwns(def.owner, camel))
            owned.append(def);
        else if (fileFits(def.path, file))
            paths.append(def);
    }
    return owned.isEmpty() ? paths : owned;
}

LangDef rubyModel(const QList<LangDef> &defs, const QString &name)
{
    const QString file = modelFile(name);
    LangDef best;
    int bestScore = 0;
    int ties = 0;
    for (const LangDef &def : defs) {
        const int score = modelScore(def, file);
        if (score == 0 || score < bestScore)
            continue;
        if (score == bestScore) {
            ++ties;
            continue;
        }
        best = def;
        bestScore = score;
        ties = 1;
    }
    return ties == 1 ? best : LangDef();
}

} // namespace

LangDef rubyChoose(const QList<LangDef> &hits, const LangSpot &spot)
{
    if (classReceiver(spot.name)) {
        const LangDef model = rubyModel(hits, spot.name);
        if (model.found())
            return model;
    }
    if (spot.receiver.contains(QLatin1Char('.')))
        return rubyChain(hits, spot);
    if (classReceiver(spot.receiver))
        return selfCall(ownedHits(hits, spot.receiver), spot.path);
    if (spot.receiver.isEmpty())
        return langPick(hits, spot.path);
    return selfCall(looseHits(hits, spot.receiver), spot.path);
}

bool rubyExtra(const LangSpot &spot, LangDef *out)
{
    if (rubyLocal(spot)) {
        *out = {};
        return true;
    }
    if (spot.name == QLatin1String("super")) {
        *out = rubySuper(spot);
        return true;
    }
    const LangDef column = schemaFind(spot.root, spot.receiver, spot.name, spot.path, spot.line);
    if (!column.found() || (column.path == spot.path && column.line == spot.line))
        return false;
    *out = column;
    return true;
}

QString rubySeek(const QString &root, const QString &path, int line, const QString &name)
{
    if (name == QLatin1String("super"))
        return rubyMethodAt(root, path, line);
    return name;
}
