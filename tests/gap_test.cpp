// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffFold.hpp"
#include "DiffParse.hpp"

#include <QDebug>
#include <QSet>

namespace {

int g_fails = 0;

void check(bool ok, const char *text, int line)
{
    if (ok)
        return;
    qCritical() << "fail" << text << "at" << line;
    ++g_fails;
}

#define CHECK(cond) check(static_cast<bool>(cond), #cond, __LINE__)

QString gapDiff()
{
    return QStringLiteral("diff --git a.cpp a.cpp\n--- a.cpp\n+++ a.cpp\n@@ -37,6 +37,6 @@\n"
                          " 37\n 38\n 39\n-40\n+changed\n 41\n 42\n");
}

FileDiff gapFile()
{
    QStringList oldLines;
    QStringList newLines;
    for (int line = 1; line <= 50; ++line) {
        oldLines.push_back(QString::number(line));
        newLines.push_back(line == 40 ? QStringLiteral("changed") : QString::number(line));
    }
    FileDiff file = parseDiff(gapDiff()).files.at(0);
    fillGaps(&file, oldLines, newLines);
    return file;
}

void testGapSkip()
{
    FileDiff file = parseDiff(gapDiff()).files.at(0);
    fillGaps(&file, {}, {});
    CHECK(file.rows.size() == 6);
}

void testGapShape()
{
    const FileDiff file = gapFile();
    CHECK(file.rows.size() == 50);
    CHECK(file.rows.at(0).leftNum == 1);
    CHECK(file.rows.at(39).kind == RowKind::Mod);
    CHECK(file.rows.at(49).leftNum == 50);
}

void testGapFold()
{
    const QList<FoldSpan> spans = foldSpans(gapFile().rows, {});
    CHECK(spans.size() == 2);
    CHECK(spans.at(0).first == 0);
    CHECK(spans.at(0).last == 35);
    CHECK(spans.at(1).first == 42);
    CHECK(spans.at(1).last == 49);
}

void testGapSearch()
{
    const FileDiff file = gapFile();
    const QList<FoldSpan> spans = foldSpans(file.rows, unfoldHits(file.rows, {}, {10}));
    CHECK(spans.size() == 1);
    CHECK(spans.at(0).first == 42);
    CHECK(foldSpans(file.rows, unfoldHits(file.rows, {}, {37})).size() == 2);
}

void testGapRest()
{
    QSet<int> open;
    for (int row = 16; row <= 35; ++row)
        open.insert(row);
    const QList<FoldSpan> rest = foldSpans(gapFile().rows, open);
    CHECK(rest.size() == 2);
    CHECK(rest.at(0).last == 15);
    CHECK(rest.at(1).first == 42);
}

void testGaps()
{
    testGapSkip();
    testGapShape();
    testGapFold();
    testGapSearch();
    testGapRest();
}

} // namespace

int gapTests()
{
    testGaps();
    return g_fails;
}
