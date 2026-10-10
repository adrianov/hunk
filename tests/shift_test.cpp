// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffParse.hpp"
#include "ReviewStore.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

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

DiffDoc diffOf(const QString &body)
{
    return parseDiff(QStringLiteral("diff --git a.rb a.rb\n--- a.rb\n+++ a.rb\n") + body);
}

bool keptAt(const ReviewStore &store, int line, bool inDiff, const char *body, int end = 0)
{
    const ReviewNote note = store.notes().value(0);
    return store.notes().size() == 1 && note.line == line && note.inDiff == inDiff
        && note.body == QLatin1String(body) && (end == 0 || note.end == end);
}

void testShiftedLine()
{
    ReviewStore store;
    store.ensure(QStringLiteral("a.rb"), false, 2, QStringLiteral("keep"));
    store.setBody(0, QStringLiteral("still valid"));
    store.sync(diffOf(QStringLiteral("@@ -1,3 +1,4 @@\n context\n+added\n keep\n tail\n")), true);
    CHECK(keptAt(store, 3, true, "still valid"));
}

void testChangedLineDrops()
{
    ReviewStore store;
    store.ensure(QStringLiteral("a.rb"), false, 1, QStringLiteral("keep"));
    store.sync(diffOf(QStringLiteral("@@ -1 +1 @@\n-keep\n+other\n")), true);
    CHECK(store.notes().isEmpty());
}

void testDuplicateDrops()
{
    ReviewStore store;
    store.ensure(QStringLiteral("a.rb"), false, 2, QStringLiteral("end"));
    store.sync(diffOf(QStringLiteral("@@ -1,4 +1,4 @@\n def\n-end\n+end!\n def\n end\n")), true);
    CHECK(store.notes().isEmpty());
}

QString shiftedRepo(const QString &root)
{
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, root);
    QFile file(QDir(root).filePath(QStringLiteral("a.rb")));
    CHECK(file.open(QIODevice::WriteOnly));
    file.write("intro\nkeep\n");
    return root;
}

QString spanRepo(const QString &root, const char *text)
{
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, root);
    QFile file(QDir(root).filePath(QStringLiteral("a.rb")));
    CHECK(file.open(QIODevice::WriteOnly));
    file.write(text);
    return root;
}

void testRangeStays()
{
    ReviewStore store;
    store.ensure(QStringLiteral("a.rb"), false, 1, QStringLiteral("one\ntwo"), 2);
    store.setBody(0, QStringLiteral("span"));
    store.sync(diffOf(QStringLiteral("@@ -1,2 +1,2 @@\n one\n two\n")), true);
    CHECK(keptAt(store, 1, true, "span"));
    CHECK(store.notes().at(0).end == 2);
}

void testRangeDrops()
{
    ReviewStore store;
    store.ensure(QStringLiteral("a.rb"), false, 1, QStringLiteral("one\ntwo"), 2);
    store.sync(diffOf(QStringLiteral("@@ -1,2 +1,2 @@\n one\n-two\n+two!\n")), true);
    CHECK(store.notes().isEmpty());
}

void testRangeFollows()
{
    ReviewStore store;
    store.ensure(QStringLiteral("a.rb"), false, 2, QStringLiteral("one\ntwo"), 3);
    store.setBody(0, QStringLiteral("span"));
    store.sync(diffOf(QStringLiteral("@@ -1,3 +1,4 @@\n context\n+added\n one\n two\n")), true);
    CHECK(keptAt(store, 3, true, "span"));
    CHECK(store.notes().at(0).end == 4);
}

void testRangeOutside()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    ReviewStore store;
    store.setRepo(spanRepo(dir.path(), "intro\none\ntwo\n"));
    store.ensure(QStringLiteral("a.rb"), false, 8, QStringLiteral("one\ntwo"), 9);
    store.setBody(0, QStringLiteral("span"));
    store.sync(diffOf(QStringLiteral("@@ -1 +1 @@\n-intro\n+intro!\n")), true);
    CHECK(keptAt(store, 2, false, "span", 3));
}

void testRangeFileChanged()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    ReviewStore store;
    store.setRepo(spanRepo(dir.path(), "intro\none\ntwo!\n"));
    store.ensure(QStringLiteral("a.rb"), false, 8, QStringLiteral("one\ntwo"), 9);
    store.sync(diffOf(QStringLiteral("@@ -1 +1 @@\n-intro\n+intro!\n")), true);
    CHECK(store.notes().isEmpty());
}

void testShiftedOutsideDiff()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    ReviewStore store;
    store.setRepo(shiftedRepo(dir.path()));
    store.ensure(QStringLiteral("a.rb"), false, 8, QStringLiteral("keep"));
    store.setBody(0, QStringLiteral("moved"));
    store.sync(diffOf(QStringLiteral("@@ -1 +1 @@\n-intro\n+intro!\n")), true);
    CHECK(keptAt(store, 2, false, "moved"));
}

} // namespace

int shiftTests()
{
    testShiftedLine();
    testChangedLineDrops();
    testDuplicateDrops();
    testRangeStays();
    testRangeDrops();
    testRangeFollows();
    testRangeOutside();
    testRangeFileChanged();
    testShiftedOutsideDiff();
    return g_fails;
}
