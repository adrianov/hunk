// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffParse.hpp"

#include "DiffDetail.hpp"

#include <QStringList>

namespace {

bool gapFits(const QStringList &lines, int at, int next)
{
    if (next < at)
        return false;
    if (lines.isEmpty())
        return next == at;
    return at >= 1 && next <= lines.size() + 1;
}

int skipped(const QStringList &lines, int at, int next)
{
    if (!gapFits(lines, at, next))
        return -1;
    if (lines.isEmpty() || next <= at)
        return 0;
    return next - at;
}

DiffRow gapRow(const QStringList &left, const QStringList &right, int leftNum, int rightNum)
{
    DiffRow row;
    row.kind = RowKind::Context;
    row.gap = true;
    row.leftNum = leftNum;
    row.rightNum = rightNum;
    row.leftText = expandTabs(leftNum > 0 ? left.at(leftNum - 1) : right.at(rightNum - 1));
    row.rightText = row.leftText;
    return row;
}

void appendGap(QList<DiffRow> *out, const QStringList &left, const QStringList &right, int leftAt, int leftNext,
               int rightAt, int rightNext)
{
    const int leftN = skipped(left, leftAt, leftNext);
    const int rightN = skipped(right, rightAt, rightNext);
    if (leftN < 0 || rightN < 0 || (leftN > 0 && rightN > 0 && leftN != rightN))
        return;
    for (int step = 0; step < qMax(leftN, rightN); ++step) {
        const int leftNum = leftN > 0 ? leftAt + step : 0;
        const int rightNum = rightN > 0 ? rightAt + step : 0;
        out->push_back(gapRow(left, right, leftNum, rightNum));
    }
}

bool gapReady(const FileDiff &file, const QStringList &left, const QStringList &right)
{
    if (file.binary || file.rows.isEmpty())
        return false;
    if (file.added)
        return !right.isEmpty();
    if (file.removed)
        return !left.isEmpty();
    return !left.isEmpty() && !right.isEmpty();
}

void placeRow(QList<DiffRow> *out, const QStringList &left, const QStringList &right, int *leftAt, int *rightAt,
              const DiffRow &row)
{
    const int leftNext = row.leftNum > 0 ? row.leftNum : *leftAt;
    const int rightNext = row.rightNum > 0 ? row.rightNum : *rightAt;
    appendGap(out, left, right, *leftAt, leftNext, *rightAt, rightNext);
    out->push_back(row);
    if (row.leftNum > 0)
        *leftAt = row.leftNum + 1;
    if (row.rightNum > 0)
        *rightAt = row.rightNum + 1;
}

int sideEnd(const QStringList &lines, int at)
{
    return lines.isEmpty() ? at : lines.size() + 1;
}

} // namespace

void fillGaps(FileDiff *file, const QStringList &left, const QStringList &right)
{
    if (!gapReady(*file, left, right))
        return;
    QList<DiffRow> out;
    int leftAt = 1;
    int rightAt = 1;
    for (const DiffRow &row : file->rows)
        placeRow(&out, left, right, &leftAt, &rightAt, row);
    appendGap(&out, left, right, leftAt, sideEnd(left, leftAt), rightAt, sideEnd(right, rightAt));
    file->rows = out;
}
