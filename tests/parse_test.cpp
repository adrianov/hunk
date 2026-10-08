// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffParse.hpp"

#include <QCoreApplication>
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

QString changedText(const QString &text, const QList<WordSpan> &spans)
{
    QString out;
    for (const WordSpan &span : spans) {
        if (span.changed)
            out += text.mid(span.start, span.end - span.start);
    }
    return out;
}

QString sampleDiff()
{
    return QStringLiteral(R"(diff --git src/a.cpp src/a.cpp
index 111..222 100644
--- src/a.cpp
+++ src/a.cpp
@@ -1,3 +1,3 @@
 int a() {
-    return 1;
+    return 2;
 }
diff --git src/b.cpp src/b.cpp
new file mode 100644
--- /dev/null
+++ src/b.cpp
@@ -0,0 +1,2 @@
+int b() {
+}
\ No newline at end of file
diff --git old.cpp new.cpp
similarity index 80%
rename from old.cpp
rename to new.cpp
--- old.cpp
+++ new.cpp
@@ -1,2 +1,2 @@
-a
-b
+c
+d
@@ -10,1 +10,1 @@
 d
diff --git "my file.cpp" "my file.cpp"
--- "my file.cpp"
+++ "my file.cpp"
@@ -2 +2 @@
-x
+y
diff --git pic.png pic.png
Binary files pic.png and pic.png differ
)");
}

void testModified(const FileDiff &file)
{
    CHECK(file.path() == QLatin1String("src/a.cpp"));
    CHECK(file.rows.size() == 3);
    CHECK(file.rows.at(0).kind == RowKind::Context);
    CHECK(file.rows.at(0).leftNum == 1);
    CHECK(file.rows.at(0).rightNum == 1);
}

void testModifiedEdit(const FileDiff &file)
{
    CHECK(file.rows.at(1).kind == RowKind::Mod);
    CHECK(file.rows.at(1).leftNum == 2);
    CHECK(file.rows.at(1).rightNum == 2);
    CHECK(changedText(file.rows.at(1).leftText, file.rows.at(1).leftSpans) == QLatin1String("1"));
    CHECK(changedText(file.rows.at(1).rightText, file.rows.at(1).rightSpans) == QLatin1String("2"));
}

void testModifiedStat(const FileDiff &file)
{
    CHECK(file.rows.at(2).leftNum == 3);
    CHECK(file.adds == 1);
    CHECK(file.dels == 1);
}

void testAdded(const FileDiff &file)
{
    CHECK(file.singlePane());
    CHECK(file.path() == QLatin1String("src/b.cpp"));
    CHECK(file.rows.size() == 2);
    CHECK(file.rows.at(0).rightNum == 1);
    CHECK(file.rows.at(1).rightNum == 2);
    CHECK(file.rows.at(0).leftNum == 0);
}

void testTrailing()
{
    const DiffDoc trailing = parseDiff(QStringLiteral("diff --git a.cpp a.cpp\n--- a.cpp\n+++ a.cpp\n@@ -1 +1 @@\n-a\n+b\n"));
    CHECK(trailing.files.size() == 1);
    CHECK(trailing.files.at(0).rows.size() == 1);
    CHECK(trailing.files.at(0).rows.at(0).kind == RowKind::Mod);
}

void testParsed(const DiffDoc &doc)
{
    testModified(doc.files.at(0));
    testModifiedEdit(doc.files.at(0));
    testModifiedStat(doc.files.at(0));
    testAdded(doc.files.at(1));
}

void testParse()
{
    const DiffDoc doc = parseDiff(sampleDiff());
    CHECK(doc.files.size() == 5);
    testParsed(doc);
    testTrailing();
}

} // namespace

DiffDoc sampleDoc()
{
    return parseDiff(sampleDiff());
}

int foldTests();
int gapTests();
int fileTests();
int reviewTests();
int shiftTests();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    testParse();
    g_fails += fileTests();
    g_fails += foldTests();
    g_fails += gapTests();
    g_fails += reviewTests();
    g_fails += shiftTests();
    Q_UNUSED(app);
    return g_fails == 0 ? 0 : 1;
}
