// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QList>
#include <QString>

class QWebEngineView;

// One side of a markdown file. Marks are its changed lines, counted from 1.
struct MdSide {
    QString text;
    QList<int> marks;
    bool added = false;
};

// GitHub Flavored Markdown, drawn with GitHub's stylesheet.
void prepareMarkdown(QWebEngineView *view);
void showMarkdown(QWebEngineView *view, const MdSide &side, const QString &dir, bool dark);

// Style and script that tint the changed blocks green on the new side and red on the old.
QString changeMarks(const MdSide &side, const QColor &ground);
