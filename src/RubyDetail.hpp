// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

#include <QHash>
#include <QList>
#include <QStringList>

int rubySkip(const QString &line, int at);
bool rubyWord(const QString &line, int at, QString *word, int *next);
bool rubyDefName(const QString &line, QString *name);
QString rubyConst(const QString &text, int start, int end);
bool rubyTypeName(const QString &line, QString *name);
bool rubyAssign(const QString &line, QString *name);
QStringList rubyLines(const QString &text);
QString rubyComment(const QStringList &lines, int line);
int rubySigEnd(const QStringList &lines, int line);
QString rubyJoin(const QStringList &lines, int first, int last);
LangDef rubyAt(const QStringList &lines, int line, const QString &path);
bool rubyClosed(const QString &line);
bool rubyDsl(const QString &line, QString *name);
QList<LangDef> rubyHits(const QString &root, const QString &name);
void rubyEnsure(const QString &root, const QString &name);
bool rubySought(const QString &root, const QString &name);
LangDef rubyPick(const QList<LangDef> &defs, const QString &path);
void noteRuby(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out);
void readSchemas(const QString &root, QHash<QString, LangDef> *out);
void schemaBump(int ticket);
void schemaStore(int ticket, const QString &root, QHash<QString, LangDef> columns);
LangDef schemaFind(const QString &root, const QString &receiver, const QString &column);
