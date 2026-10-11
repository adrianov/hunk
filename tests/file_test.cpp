// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDetail.hpp"
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

void testRenameTitle()
{
    FileDiff file;
    file.oldPath = QStringLiteral("lib/metrics/build.rb");
    file.newPath = QStringLiteral("lib/metrics/base.rb");
    CHECK(file.title() == QStringLiteral("lib/metrics/build.rb → base.rb"));
    CHECK(file.shortName() == QStringLiteral("build.rb → base.rb"));
}

DiffDoc splitRename()
{
    return parseDiff(QStringLiteral(R"(diff --git build.rb build.rb
deleted file mode 100644
--- build.rb
+++ /dev/null
@@ -1 +0,0 @@
-old
diff --git base.rb base.rb
new file mode 100644
--- /dev/null
+++ base.rb
@@ -0,0 +1 @@
+new
)"));
}

void testJoinRename()
{
    DiffDoc doc = splitRename();
    CHECK(joinRename(&doc, QStringLiteral("build.rb"), QStringLiteral("base.rb"), QStringLiteral(R"(diff --git build.rb base.rb
--- build.rb
+++ base.rb
@@ -1 +1 @@
-old
+new
)")));
    const FileDiff file = doc.files.value(0);
    CHECK(doc.files.size() == 1 && file.title() == QStringLiteral("build.rb → base.rb"));
    CHECK(!file.added && !file.removed && file.rows.size() == 1);
}

FileDiff samplePlain()
{
    return unchangedFile(QStringLiteral("lib/app.rb"), {QStringLiteral("def hi"), QStringLiteral("end")});
}

void testUnchangedFile()
{
    const FileDiff file = samplePlain();
    CHECK(file.adds == 0);
    CHECK(file.dels == 0);
    CHECK(file.rows.size() == 2);
    CHECK(file.rows.at(0).kind == RowKind::Context);
    CHECK(file.rows.at(0).rightNum == 1);
    CHECK(file.rows.at(0).leftText == file.rows.at(0).rightText);
}

void testUnchangedRow()
{
    const FileDiff file = samplePlain();
    bool oldSide = true;
    CHECK(rowForLine(file, 2, &oldSide) == 1);
    CHECK(!oldSide);
    DiffDoc doc;
    doc.files.append(file);
    CHECK(fileNamed(doc, QStringLiteral("lib/app.rb")) == 0);
    CHECK(fileNamed(doc, QStringLiteral("missing.rb")) < 0);
}

void testDedent()
{
    CHECK(dedent(QStringLiteral("  # note\n  def hi\n\n    go\n  end")) == QStringLiteral("# note\ndef hi\n\n  go\nend"));
    CHECK(dedent(QStringLiteral("\tdef hi\n\t  go")) == QStringLiteral("def hi\n  go"));
    CHECK(dedent(QStringLiteral("  def hi\n    go\n…")) == QStringLiteral("def hi\n  go\n…"));
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
    testRenameTitle();
    testJoinRename();
    testUnchangedFile();
    testUnchangedRow();
    testDedent();
    return g_fails;
}
