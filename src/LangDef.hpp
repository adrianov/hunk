// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QPointer>
#include <QString>

#include <functional>

class QObject;

// Where a hovered name is defined. Every language fills these fields.
// A language layer adds names, exclusions, and ranking. The index and the walk stay shared.
struct LangDef {
    QString path;
    int line = 0;
    QString comment;
    QString signature;
    QString body;
    QString owner;

    bool found() const { return line > 0 && !path.isEmpty(); }
};

// One name under the cursor. `line` is the source line, `column` the character on that line.
struct LangSpot {
    QString root;
    QString path;
    int line = 0;
    int column = 0;
    QString name;
    QString receiver;
};

// Definition of the name at `spot`, or an empty result when that language has none.
LangDef lookupDef(const LangSpot &spot);
LangSpot defSpot(const QString &root, const QString &path, int line, int column, const QString &name,
                 const QString &receiver = {});
// Remembers the project root. A new root clears every language cache. `done` runs on `watch`.
void warmLang(const QString &root, QPointer<QObject> watch = {}, std::function<void()> done = {});
bool langReady(const QString &root, const QString &path);
