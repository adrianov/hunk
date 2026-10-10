// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffParse.hpp"
#include "ReviewExport.hpp"

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

void testExportOld()
{
    ReviewNote note;
    note.path = QStringLiteral("src/a.cpp");
    note.line = 2;
    note.snippet = QStringLiteral("    return 2;");
    note.body = QStringLiteral("Use a named constant.\n");
    ReviewNote oldNote;
    oldNote.path = QStringLiteral("src/a.cpp");
    oldNote.oldSide = true;
    oldNote.line = 4;
    oldNote.snippet = QStringLiteral("old line");
    oldNote.body = QStringLiteral("Still used.");
    CHECK(reviewMarkdown(QStringLiteral("main vs origin/main"), {note, oldNote}) == QStringLiteral(
        "# Review: main vs origin/main\n"
        "\nFile references are `path:line` on the new side. `(old)` means the line number before the change.\n"
        "\n## `src/a.cpp:2`\n"
        ">     return 2;\n"
        "\nUse a named constant.\n"
        "\n## `src/a.cpp:4` (old)\n"
        "> old line\n"
        "\nStill used.\n"));
}

void testExportPlain()
{
    ReviewNote note;
    note.path = QStringLiteral("src/a.cpp");
    note.line = 2;
    note.snippet = QStringLiteral("    return 2;");
    note.body = QStringLiteral("Use a named constant.\n");
    const QString plain = reviewMarkdown(QStringLiteral("t"), {note});
    CHECK(plain.contains(QStringLiteral("`path:line`")));
    CHECK(!plain.contains(QStringLiteral("(old) means")));
    CHECK(reviewMarkdown(QStringLiteral("t"), {ReviewNote{}}).isEmpty());
}

void testExportRange()
{
    ReviewNote note;
    note.path = QStringLiteral("src/app.rb");
    note.line = 2;
    note.end = 4;
    note.snippet = QStringLiteral("one\ntwo\nthree");
    note.body = QStringLiteral("All three.");
    const QString text = reviewMarkdown(QStringLiteral("t"), {note});
    CHECK(text.contains(QStringLiteral("`src/app.rb:2-4`")));
    CHECK(text.contains(QStringLiteral("> one\n> two\n> three\n")));
    CHECK(text.contains(QStringLiteral("`path:first-last`")));
}

void testExport()
{
    testExportOld();
    testExportPlain();
    testExportRange();
}

DiffRow wordRow(const char *body)
{
    const QString diff = QStringLiteral("diff --git a.rb a.rb\n--- a.rb\n+++ a.rb\n@@ -1 +1 @@\n")
        + QString::fromUtf8(body);
    return parseDiff(diff).files.at(0).rows.at(0);
}

void testWordRewritten()
{
    const DiffRow row = wordRow("-# one two three\n+# four five six\n");
    CHECK(row.leftSpans.isEmpty());
    CHECK(row.rightSpans.isEmpty());
}

void testWordEdited()
{
    const DiffRow row = wordRow("-keep this\n+keep that\n");
    CHECK(changedText(row.leftText, row.leftSpans) == QLatin1String("this"));
    CHECK(changedText(row.rightText, row.rightSpans) == QLatin1String("that"));
}

void testReview()
{
    testExport();
    testWordRewritten();
    testWordEdited();
}

} // namespace

int reviewTests()
{
    testReview();
    return g_fails;
}
