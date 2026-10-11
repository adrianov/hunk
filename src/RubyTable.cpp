// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>

namespace {

QString tableOf(const LangDef &def)
{
    return def.comment.startsWith(QLatin1String("# ")) ? def.comment.mid(2) : QString();
}

bool modelPath(const QString &path, QString *rest)
{
    const QString mark = QStringLiteral("app/models/");
    const int at = path.lastIndexOf(mark);
    if (at < 0 || (at > 0 && path.at(at - 1) != QLatin1Char('/')))
        return false;
    *rest = path.mid(at + mark.size());
    return true;
}

bool nameChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool edge(const QString &line, int at)
{
    return at < 0 || at >= line.size() || !nameChar(line.at(at));
}

bool wordHit(const QString &line, const QString &word)
{
    if (word.isEmpty())
        return false;
    int from = 0;
    while (from < line.size()) {
        const int at = line.indexOf(word, from);
        if (at < 0)
            return false;
        if (edge(line, at - 1) && edge(line, at + word.size()))
            return true;
        from = at + word.size();
    }
    return false;
}

int columnScore(const LangDef &def, const QString &model, const QString &line)
{
    const QString table = tableOf(def);
    int score = 0;
    if (!model.isEmpty() && table == model)
        score += 3;
    if (wordHit(line, table))
        score += 4;
    return score;
}

} // namespace

QString modelTable(const QString &path)
{
    QString rest;
    if (!modelPath(path, &rest) || rest.contains(QLatin1Char('/')) || !rest.endsWith(QLatin1String(".rb")))
        return {};
    return rubyPlural(rest.chopped(3));
}

QString lineAt(const QString &root, const QString &path, int line)
{
    if (line <= 0 || path.isEmpty())
        return {};
    QFile file(QDir(root).filePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QStringList lines = rubyLines(QString::fromUtf8(file.readAll()));
    return line > lines.size() ? QString() : lines.at(line - 1);
}

LangDef bestColumn(const QList<LangDef> &hits, const QString &model, const QString &line)
{
    LangDef best;
    int bestScore = 0;
    int ties = 0;
    for (const LangDef &def : hits) {
        const int score = columnScore(def, model, line);
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
