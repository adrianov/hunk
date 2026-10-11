// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

#include <QList>
#include <QStringList>

struct RubyPlace {
    QString path;
    int line = 0;
};

int rubySkip(const QString &line, int at);
bool rubyWord(const QString &line, int at, QString *word, int *next);
bool rubyDefName(const QString &line, QString *name);
QStringList rubyLines(const QString &text);
QString rubyComment(const QStringList &lines, int line);
int rubySigEnd(const QStringList &lines, int line);
QString rubyJoin(const QStringList &lines, int first, int last);
LangDef rubyAt(const QStringList &lines, int line, const QString &path);
QString rubyRead(const QString &path);
QList<RubyPlace> rubyHits(const QString &root, const QString &name);
RubyPlace rubyPick(const QList<RubyPlace> &places, const QString &path);
