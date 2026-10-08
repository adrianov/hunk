// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffFold.hpp"

#include <QDebug>

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

QList<DiffRow> rowsOf(const QString &kinds)
{
    QList<DiffRow> rows;
    for (const QChar kind : kinds) {
        DiffRow row;
        row.kind = kind == QLatin1Char('c') ? RowKind::Context : RowKind::Add;
        rows.push_back(row);
    }
    return rows;
}

void testFoldShort()
{
    CHECK(foldSpans(rowsOf(QStringLiteral("accccca")), {}).isEmpty());
}

void testFoldInside()
{
    const QString block = QStringLiteral("a") + QString(20, QLatin1Char('c')) + QStringLiteral("a");
    const QList<FoldSpan> spans = foldSpans(rowsOf(block), {});
    CHECK(spans.size() == 1);
    CHECK(spans.at(0).first == 4);
    CHECK(spans.at(0).last == 17);
}

void testFoldLead()
{
    const QList<FoldSpan> spans = foldSpans(rowsOf(QString(20, QLatin1Char('c')) + QStringLiteral("a")), {});
    CHECK(spans.size() == 1);
    CHECK(spans.at(0).first == 0);
    CHECK(spans.at(0).last == 16);
}

void testFoldBlock()
{
    testFoldInside();
    testFoldLead();
}

void testFoldMiddle()
{
    const int count = kFoldWide + 20;
    const QString pattern = QStringLiteral("a") + QString(count, QLatin1Char('c')) + QStringLiteral("a");
    const QList<FoldSpan> spans = foldSpans(rowsOf(pattern), {});
    CHECK(spans.size() == 1);
    CHECK(spans.at(0).first == 4);
    CHECK(spans.at(0).last == count - 3);
}

void testFoldEdge()
{
    const int count = kFoldWide + 20;
    const QList<FoldSpan> spans = foldSpans(rowsOf(QString(count, QLatin1Char('c')) + QStringLiteral("a")), {});
    CHECK(spans.size() == 1);
    CHECK(spans.at(0).first == 0);
    CHECK(spans.at(0).last == count - 4);
}

void testFoldOpen()
{
    const int count = kFoldWide + 20;
    const QString pattern = QStringLiteral("a") + QString(count, QLatin1Char('c')) + QStringLiteral("a");
    const QList<FoldSpan> spans = foldSpans(rowsOf(pattern), {10});
    CHECK(spans.size() == 2);
    CHECK(spans.at(0).last == 9);
    CHECK(spans.at(1).first == 11);
}

void testFoldPlain()
{
    CHECK(foldSpans(rowsOf(QString(30, QLatin1Char('c'))), {}).isEmpty());
}

} // namespace

int foldTests()
{
    testFoldShort();
    testFoldBlock();
    testFoldMiddle();
    testFoldEdge();
    testFoldOpen();
    testFoldPlain();
    return g_fails;
}
