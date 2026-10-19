// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

#include <QHash>
#include <QList>
#include <QStringList>

int rubySkip(const QString &line, int at);
bool rubyWordChar(QChar ch);
int rubyWordEnd(const QString &text, int at);
bool rubyWordHit(const QString &line, int at, const QString &name);
bool rubyEndLine(const QString &line);
int rubyIndent(const QString &line);
bool rubyWord(const QString &line, int at, QString *word, int *next);
QString rubyReceiver(const QString &text, int start);
bool rubyOwns(const QString &owner, const QString &name);
void rubyNoteBuilt(const QString &text, QStringList *types, QStringList *calls);
void rubyFactory(const QString &root, const QString &text, QStringList *types);
QString rubyLead(const LangSpot &spot, const QString &line = {});
QString rubyTyped(const LangSpot &spot, const QString &line);

// One read of the hovered file and the method that encloses that line.
struct RubyBound {
    QStringList lines;
    int opened = 0;
    int last = 0;
};

QStringList readLines(const QString &root, const QString &path);
int defLine(const QStringList &lines, int line);
int methodLast(const QStringList &lines, int opened);
bool defSig(const LangDef &def);
QString constBefore(const QString &text, int dot);
bool keepOwner(QString *found, const QString &owner);
bool plainName(const QString &name);
RubyBound rubyBound(const LangSpot &spot);
QString methodType(const QString &root, const QString &name);
bool finderWord(const QString &word);
QString wordAt(const QString &text, int at);
QString followText(const QString &root, const QString &text, const QString &nest, const QString &method);
QString rubyFollow(const LangSpot &spot, const RubyBound &bound);
QStringList linesFor(const QString &root, const QString &path);
void rubyForgetLines();
LangDef rubyChain(const QList<LangDef> &hits, const LangSpot &spot);
bool rubyDefName(const QString &line, QString *name);
QString rubyConst(const QString &text, int start, int end);
QString rubyParentName(const QString &line);
bool rubyTypeName(const QString &line, QString *name);
bool rubyAssign(const QString &line, QString *name);
// Drops the body of the last `name` constant and files it under each scope of `owner`:
// in `A::B`, also as `A::B::NAME` and `B::NAME`.
void rubyConstDef(const QString &owner, const QString &name, QHash<QString, QList<LangDef>> *out);
QStringList rubyLines(const QString &text);
QString rubyComment(const QStringList &lines, int line);
int rubySigEnd(const QStringList &lines, int line);
QString rubyJoin(const QStringList &lines, int first, int last);
LangDef rubyAt(const QStringList &lines, int line, const QString &path);
bool rubyClosed(const QString &line);
bool rubyDsl(const QString &line, QString *name);
void rubyWrap(LangDef *def, const QStringList &opens);
void noteRuby(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out);
void rubyUses(const QStringList &lines, const QString &path, const QStringList &names,
              QHash<QString, QList<LangDef>> *out);
bool rubyForeignCall(const QString &line, const QString &name, const QString &owner);
void readSchemas(const QString &root, QHash<QString, LangDef> *out);
void schemaBump(int ticket);
void schemaStore(int ticket, const QString &root, QHash<QString, LangDef> columns);
QString rubyPlural(const QString &name);
QString modelTable(const QString &path);
QString lineAt(const QString &root, const QString &path, int line);
LangDef bestColumn(const QList<LangDef> &hits, const QString &model, const QString &line);
LangDef schemaFind(const QString &root, const QString &receiver, const QString &column, const QString &path, int line);
bool rubyMixin(const QString &line, QString *name);
bool rubyCallMix(const QString &line, QString *module, QString *host);
QStringList rubyMixed(const QString &tail);
void rubyAddHost(const QString &module, const QString &host);
void rubyResetMix();
QStringList rubyHosts(const QString &module);
bool rubyAssigned(const QString &line, const QString &name);
bool rubyBlockArg(const QString &line, const QString &name);
bool rubyLocal(const LangSpot &spot);
QString rubyMethodAt(const QString &root, const QString &path, int line);
LangDef joined(QList<LangDef> hits);
LangDef ancestorDef(const QString &root, const QString &parent, const QList<LangDef> &hits, const QString &path, int defLine);
QList<LangDef> hostDefs(const QString &root, const QString &module, const QList<LangDef> &hits, const QString &path, int defLine);
LangDef rubySuper(const LangSpot &spot);
