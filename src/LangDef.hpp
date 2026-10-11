// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QPointer>
#include <QString>

#include <functional>

class QObject;

// Where a hovered name is defined. Each language fills the same fields.
// Ruby is built in. C++, Swift, JavaScript, TypeScript, and Objective-C use this shape too.
struct LangDef {
    QString path;
    int line = 0;
    QString comment;
    QString signature;
    QString body;

    bool found() const { return line > 0 && !path.isEmpty(); }
};

// One name under the cursor. `line` is the source line, `column` the character on that line.
struct LangSpot {
    QString root;
    QString path;
    int line = 0;
    int column = 0;
    QString name;
};

// Definition of the name at `spot`, or an empty result when this language has none yet.
LangDef lookupDef(const LangSpot &spot);
// Builds the Ruby name index. When a walk is stored, `done` is queued on `watch`.
void warmLang(const QString &root, QPointer<QObject> watch = {}, std::function<void()> done = {});
bool langReady(const QString &root, const QString &path);
