// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"
#include "SyntaxRule.hpp"

#include <QHash>
#include <QStringList>

// One language on the shared index. Null hooks keep the common walk, pick, and hover.
struct LangTune {
    Lang lang = Lang::None;
    bool method = false;
    bool variable = false;
    bool type = false;
    bool keyword = false;
    QString (*name)(const QString &text, int start, int end, SynKind kind) = nullptr;
    QString (*seek)(const QString &root, const QString &path, int line, const QString &name) = nullptr;
    bool (*extra)(const LangSpot &spot, LangDef *out) = nullptr;
    LangDef (*choose)(const QList<LangDef> &hits, const LangSpot &spot) = nullptr;
    void (*note)(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out) = nullptr;
    void (*use)(const QStringList &lines, const QString &path, const QStringList &names,
                QHash<QString, QList<LangDef>> *out) = nullptr;
    void (*reset)(int ticket) = nullptr;
};

const LangTune *langTune(Lang lang);
bool tuneKind(const LangTune *tune, SynKind kind);
QString langSeek(const QString &root, const QString &path, int line, const QString &name);
void langReset(int ticket);

QList<LangDef> langHits(const QString &root, const QString &name, Lang lang);
QList<LangDef> langUses(const QString &root, const QString &name);
void langEnsure(const QString &root, const QString &name);
bool langSought(const QString &root, const QString &name);
bool langCached(const QString &root);
LangDef langPick(const QList<LangDef> &defs, const QString &path);
void langWalk(const QString &root, const QStringList &names, QHash<QString, QList<LangDef>> *out,
             QHash<QString, QList<LangDef>> *uses);
