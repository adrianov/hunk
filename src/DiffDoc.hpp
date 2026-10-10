// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QList>
#include <QString>

// One word-level highlight range inside a diff line.
struct WordSpan {
    int start = 0;
    int end = 0;
    bool changed = false;
};

enum class SynKind { Plain, Comment, String, Number, Keyword, Type, Method, Variable, Strong, Emph, Strike };

// One syntax-colored range inside a diff line.
struct SynSpan {
    int start = 0;
    int end = 0;
    SynKind kind = SynKind::Plain;
};

// One visual line after a row is wrapped to the pane width.
struct WrapSpan {
    int start = 0;
    int end = 0;
};

// One painted run: syntax color, word change, and a wrap break.
struct Piece {
    int start = 0;
    int end = 0;
    SynKind kind = SynKind::Plain;
    bool changed = false;
    bool newLine = false;
};

enum class RowKind { Context, Add, Del, Mod };

// How strongly a changed line is painted against the last review.
enum class RowHeat { Open, Seen, Fresh };

// One visual row. A modification pairs an old line with a new line.
struct DiffRow {
    RowKind kind = RowKind::Context;
    bool gap = false;
    int leftNum = 0;
    int rightNum = 0;
    QString leftText;
    QString rightText;
    QList<WordSpan> leftSpans;
    QList<WordSpan> rightSpans;
    QList<SynSpan> leftSyn;
    QList<SynSpan> rightSyn;
    QList<Piece> leftPiece;
    QList<Piece> rightPiece;
    RowHeat heat = RowHeat::Open;
};

// One file from a unified diff.
struct FileDiff {
    QString oldPath;
    QString newPath;
    bool added = false;
    bool removed = false;
    bool binary = false;
    int adds = 0;
    int dels = 0;
    QList<DiffRow> rows;

    QString path() const
    {
        if (!newPath.isEmpty() && newPath != QLatin1String("/dev/null"))
            return newPath;
        return oldPath;
    }

    QString title() const
    {
        const bool oldOk = !oldPath.isEmpty() && oldPath != QLatin1String("/dev/null");
        const bool newOk = !newPath.isEmpty() && newPath != QLatin1String("/dev/null");
        if (oldOk && newOk && oldPath != newPath)
            return oldPath + QStringLiteral(" → ") + newPath;
        return path();
    }

    QString shortName() const;

    QString folder() const
    {
        const QString p = path();
        const int slash = p.lastIndexOf(QLatin1Char('/'));
        return slash >= 0 ? p.left(slash) : QString();
    }

    bool singlePane() const
    {
        if (removed || binary || rows.isEmpty())
            return false;
        if (added)
            return true;
        for (const DiffRow &row : rows) {
            if (row.kind != RowKind::Add)
                return false;
        }
        return true;
    }
};

struct DiffDoc {
    QList<FileDiff> files;
};

// File indexes in the same order as the file list: root first, then folders.
QList<int> listedOrder(const DiffDoc &doc);

struct LineHit {
    int file = -1;
    int row = -1;
};
