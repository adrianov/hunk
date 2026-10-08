// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffParse.hpp"

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

void testRenamed(const FileDiff &file)
{
    CHECK(file.title() == QStringLiteral("old.cpp → new.cpp"));
    CHECK(file.rows.size() == 3);
    CHECK(file.rows.at(0).kind == RowKind::Mod);
    CHECK(file.rows.at(0).leftText == QLatin1String("a"));
    CHECK(file.rows.at(0).rightText == QLatin1String("c"));
}

void testRenamedRest(const FileDiff &file)
{
    CHECK(file.rows.at(1).leftText == QLatin1String("b"));
    CHECK(file.rows.at(1).rightText == QLatin1String("d"));
    CHECK(file.rows.at(2).kind == RowKind::Context);
    CHECK(file.rows.at(2).leftNum == 10);
    CHECK(file.rows.at(2).rightText == QLatin1String("d"));
}

void testQuoted(const DiffDoc &doc)
{
    const FileDiff &file = doc.files.at(3);
    CHECK(file.path() == QLatin1String("my file.cpp"));
    CHECK(file.rows.size() == 1);
    CHECK(file.rows.at(0).kind == RowKind::Mod);
    CHECK(file.rows.at(0).leftNum == 2);
    CHECK(findLine(doc, QStringLiteral("my file.cpp"), false, 2).row == 0);
}

void testBinary(const FileDiff &file)
{
    CHECK(file.binary);
    CHECK(file.rows.isEmpty());
}

} // namespace

DiffDoc sampleDoc();

int fileTests()
{
    const DiffDoc doc = sampleDoc();
    testRenamed(doc.files.at(2));
    testRenamedRest(doc.files.at(2));
    testQuoted(doc);
    testBinary(doc.files.at(4));
    return g_fails;
}
