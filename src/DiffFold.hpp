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
constexpr int kFoldWide = 500;

// Hidden stretches of unchanged lines. Rows in `open` stay visible.
// When lines outside a block were loaded, a block of at most kFoldWide unchanged lines stays open.
// Longer blocks, lines outside the block, and a file whose outside lines were not loaded collapse.
QList<FoldSpan> foldSpans(const QList<DiffRow> &rows, const QSet<int> &open);

// `open`, plus every folded stretch that contains a hit, so those lines stay visible.
QSet<int> unfoldHits(const QList<DiffRow> &rows, const QSet<int> &open, const QSet<int> &hits);
