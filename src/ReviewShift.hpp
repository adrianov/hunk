// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "ReviewNote.hpp"

#include <QHash>
#include <QStringList>

// Worktree lines read once per path during a sync.
struct LineCache {
    QHash<QString, QStringList> files;
};

// Line number where the commented text now sits, or 0 when it did not move.
int shiftedLine(const DiffDoc &doc, const QString &root, const ReviewNote &note, LineCache *cache);

// Worktree text for one path, read once and kept on the cache.
QStringList cachedFile(const DiffDoc &doc, const QString &root, const QString &path, LineCache *cache);

// Line where a span now starts, or 0 when it did not move or the interval is not unique.
int spanShift(const DiffDoc &doc, const QString &root, const ReviewNote &note, LineCache *cache);
