// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffFold.hpp"

namespace {

constexpr int kKeep = 3;
constexpr int kMin = 8;

void hideRange(QList<FoldSpan> *out, int from, int to, const QSet<int> &open)
{
    if (to - from < kMin)
        return;
    int cursor = from;
    while (cursor < to) {
        if (open.contains(cursor)) {
            ++cursor;
            continue;
        }
        const int first = cursor;
        while (cursor < to && !open.contains(cursor))
            ++cursor;
        if (cursor > first)
            out->push_back(FoldSpan{first, cursor - 1});
    }
}

// Keep a few unchanged lines beside each change. Hide the rest of a long run.
void foldRun(QList<FoldSpan> *out, int begin, int end, int total, const QSet<int> &open, const QSet<int> &shown)
{
    if (begin == 0 && end == total)
        return;
    const int lead = begin > 0 && !shown.contains(begin - 1) ? kKeep : 0;
    const int tail = end < total && !shown.contains(end) ? kKeep : 0;
    const int from = begin + lead;
    const int to = end - tail;
    if (from < to)
        hideRange(out, from, to, open);
}

int blockEnd(const QList<DiffRow> &rows, int begin, int *context)
{
    int index = begin;
    while (index < rows.size() && !rows.at(index).gap) {
        if (rows.at(index).kind == RowKind::Context)
            ++*context;
        ++index;
    }
    return index;
}

void pinBlock(QSet<int> *shown, const QList<DiffRow> &rows, int begin, int end)
{
    for (int row = begin; row < end; ++row) {
        if (rows.at(row).kind == RowKind::Context)
            shown->insert(row);
    }
}

bool hasGap(const QList<DiffRow> &rows)
{
    for (const DiffRow &row : rows) {
        if (row.gap)
            return true;
    }
    return false;
}

QSet<int> shownRows(const QList<DiffRow> &rows)
{
    QSet<int> shown;
    if (!hasGap(rows))
        return shown;
    int index = 0;
    while (index < rows.size()) {
        if (rows.at(index).gap) {
            ++index;
            continue;
        }
        int context = 0;
        const int end = blockEnd(rows, index, &context);
        if (context <= kFoldWide)
            pinBlock(&shown, rows, index, end);
        index = end;
    }
    return shown;
}

} // namespace

QList<FoldSpan> foldSpans(const QList<DiffRow> &rows, const QSet<int> &open)
{
    const QSet<int> shown = shownRows(rows);
    QList<FoldSpan> spans;
    int index = 0;
    while (index < rows.size()) {
        if (rows.at(index).kind != RowKind::Context || shown.contains(index)) {
            ++index;
            continue;
        }
        const int begin = index;
        while (index < rows.size() && rows.at(index).kind == RowKind::Context && !shown.contains(index))
            ++index;
        foldRun(&spans, begin, index, rows.size(), open, shown);
    }
    return spans;
}
