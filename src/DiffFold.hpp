// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QSet>

// One hidden stretch of unchanged rows, inclusive.
struct FoldSpan {
    int first = 0;
    int last = 0;
};

constexpr int kFoldStep = 20;
constexpr int kFoldEdge = 64;

// Hidden stretches of unchanged lines. Rows in `open` stay visible.
QList<FoldSpan> foldSpans(const QList<DiffRow> &rows, const QSet<int> &open);
