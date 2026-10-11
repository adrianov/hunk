// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

#include <QHash>
#include <QList>
#include <QStringList>

int rubySkip(const QString &line, int at);
bool rubyWordChar(QChar ch);
bool rubyWordHit(const QString &line, int at, const QString &name);
int rubyIndent(const QString &line);
bool rubyWord(const QString &line, int at, QString *word, int *next);
bool rubyDefName(const QString &line, QString *name);
QString rubyConst(const QString &text, int start, int end);
QString rubyParentName(const QString &line);
bool rubyTypeName(const QString &line, QString *name);
bool rubyAssign(const QString &line, QString *name);
QStringList rubyLines(const QString &text);
QString rubyComment(const QStringList &lines, int line);
int rubySigEnd(const QStringList &lines, int line);
QString rubyJoin(const QStringList &lines, int first, int last);
LangDef rubyAt(const QStringList &lines, int line, const QString &path);
bool rubyClosed(const QString &line);
bool rubyDsl(const QString &line, QString *name);
void rubyWrap(LangDef *def, const QStringList &opens);
void noteRuby(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out);
void readSchemas(const QString &root, QHash<QString, LangDef> *out);
void schemaBump(int ticket);
void schemaStore(int ticket, const QString &root, QHash<QString, LangDef> columns);
QString rubyPlural(const QString &name);
QString modelTable(const QString &path);
QString lineAt(const QString &root, const QString &path, int line);
LangDef bestColumn(const QList<LangDef> &hits, const QString &model, const QString &line);
LangDef schemaFind(const QString &root, const QString &receiver, const QString &column, const QString &path, int line);
bool rubyMixin(const QString &line, QString *name);
void rubyAddHost(const QString &module, const QString &host);
void rubyResetMix();
QStringList rubyHosts(const QString &module);
bool rubyAssigned(const QString &line, const QString &name);
void rubyForgetLocal();
bool rubyLocal(const LangSpot &spot);
QString rubyMethodAt(const QString &root, const QString &path, int line);
LangDef joined(QList<LangDef> hits);
LangDef ancestorDef(const QString &root, const QString &parent, const QList<LangDef> &hits, const QString &path, int defLine);
QList<LangDef> hostDefs(const QString &root, const QString &module, const QList<LangDef> &hits, const QString &path, int defLine);
LangDef rubySuper(const LangSpot &spot);
