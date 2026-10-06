#include "DiffParse.hpp"
#include "ReviewExport.hpp"

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

void testParse()
{
    const QString diff =
        QStringLiteral("diff --git src/a.cpp src/a.cpp\n")
        + QStringLiteral("index 111..222 100644\n")
        + QStringLiteral("--- src/a.cpp\n")
        + QStringLiteral("+++ src/a.cpp\n")
        + QStringLiteral("@@ -1,3 +1,3 @@\n")
        + QStringLiteral(" int a() {\n")
        + QStringLiteral("-    return 1;\n")
        + QStringLiteral("+    return 2;\n")
        + QStringLiteral(" }\n")
        + QStringLiteral("diff --git src/b.cpp src/b.cpp\n")
        + QStringLiteral("new file mode 100644\n")
        + QStringLiteral("--- /dev/null\n")
        + QStringLiteral("+++ src/b.cpp\n")
        + QStringLiteral("@@ -0,0 +1,2 @@\n")
        + QStringLiteral("+int b() {\n")
        + QStringLiteral("+}\n")
        + QStringLiteral("\\ No newline at end of file\n")
        + QStringLiteral("diff --git old.cpp new.cpp\n")
        + QStringLiteral("similarity index 80%\n")
        + QStringLiteral("rename from old.cpp\n")
        + QStringLiteral("rename to new.cpp\n")
        + QStringLiteral("--- old.cpp\n")
        + QStringLiteral("+++ new.cpp\n")
        + QStringLiteral("@@ -1,2 +1,2 @@\n")
        + QStringLiteral("-a\n")
        + QStringLiteral("-b\n")
        + QStringLiteral("+c\n")
        + QStringLiteral("+d\n")
        + QStringLiteral("@@ -10,1 +10,1 @@\n")
        + QStringLiteral(" d\n")
        + QStringLiteral("diff --git \"my file.cpp\" \"my file.cpp\"\n")
        + QStringLiteral("--- \"my file.cpp\"\n")
        + QStringLiteral("+++ \"my file.cpp\"\n")
        + QStringLiteral("@@ -2 +2 @@\n")
        + QStringLiteral("-x\n")
        + QStringLiteral("+y\n")
        + QStringLiteral("diff --git pic.png pic.png\n")
        + QStringLiteral("Binary files pic.png and pic.png differ\n");

    const DiffDoc doc = parseDiff(diff);
    CHECK(doc.files.size() == 5);

    const FileDiff &modified = doc.files.at(0);
    CHECK(modified.path() == QLatin1String("src/a.cpp"));
    CHECK(modified.rows.size() == 3);
    CHECK(modified.rows.at(0).kind == RowKind::Context);
    CHECK(modified.rows.at(0).leftNum == 1);
    CHECK(modified.rows.at(0).rightNum == 1);
    CHECK(modified.rows.at(1).kind == RowKind::Mod);
    CHECK(modified.rows.at(1).leftNum == 2);
    CHECK(modified.rows.at(1).rightNum == 2);
    CHECK(changedText(modified.rows.at(1).leftText, modified.rows.at(1).leftSpans) == QLatin1String("1"));
    CHECK(changedText(modified.rows.at(1).rightText, modified.rows.at(1).rightSpans) == QLatin1String("2"));
    CHECK(modified.rows.at(2).leftNum == 3);
    CHECK(modified.adds == 1);
    CHECK(modified.dels == 1);

    const FileDiff &added = doc.files.at(1);
    CHECK(added.singlePane());
    CHECK(added.path() == QLatin1String("src/b.cpp"));
    CHECK(added.rows.size() == 2);
    CHECK(added.rows.at(0).rightNum == 1);
    CHECK(added.rows.at(1).rightNum == 2);
    CHECK(added.rows.at(0).leftNum == 0);

    const FileDiff &renamed = doc.files.at(2);
    CHECK(renamed.title() == QStringLiteral("old.cpp → new.cpp"));
    CHECK(renamed.rows.size() == 3);
    CHECK(renamed.rows.at(0).kind == RowKind::Mod);
    CHECK(renamed.rows.at(0).leftText == QLatin1String("a"));
    CHECK(renamed.rows.at(0).rightText == QLatin1String("c"));
    CHECK(renamed.rows.at(1).leftText == QLatin1String("b"));
    CHECK(renamed.rows.at(1).rightText == QLatin1String("d"));
    CHECK(renamed.rows.at(2).kind == RowKind::Context);
    CHECK(renamed.rows.at(2).leftNum == 10);
    CHECK(renamed.rows.at(2).rightText == QLatin1String("d"));

    const FileDiff &quoted = doc.files.at(3);
    CHECK(quoted.path() == QLatin1String("my file.cpp"));
    CHECK(quoted.rows.size() == 1);
    CHECK(quoted.rows.at(0).kind == RowKind::Mod);
    CHECK(quoted.rows.at(0).leftNum == 2);
    CHECK(findLine(doc, QStringLiteral("my file.cpp"), false, 2).row == 0);

    CHECK(doc.files.at(4).binary);
    CHECK(doc.files.at(4).rows.isEmpty());

    const DiffDoc trailing = parseDiff(QStringLiteral("diff --git a.cpp a.cpp\n--- a.cpp\n+++ a.cpp\n@@ -1 +1 @@\n-a\n+b\n"));
    CHECK(trailing.files.size() == 1);
    CHECK(trailing.files.at(0).rows.size() == 1);
    CHECK(trailing.files.at(0).rows.at(0).kind == RowKind::Mod);
}

void testExport()
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
    const QString markdown = reviewMarkdown(QStringLiteral("main vs origin/main"), {note, oldNote});
    const QString expected =
        QStringLiteral("# Review: main vs origin/main\n")
        + QStringLiteral("\nFile references are `path:line` on the new side. `(old)` means the line number before the change.\n")
        + QStringLiteral("\n## `src/a.cpp:2`\n")
        + QStringLiteral(">     return 2;\n")
        + QStringLiteral("\nUse a named constant.\n")
        + QStringLiteral("\n## `src/a.cpp:4` (old)\n")
        + QStringLiteral("> old line\n")
        + QStringLiteral("\nStill used.\n");
    CHECK(markdown == expected);
    CHECK(reviewMarkdown(QStringLiteral("t"), {ReviewNote{}}).isEmpty());
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    testParse();
    testExport();
    Q_UNUSED(app);
    return g_fails == 0 ? 0 : 1;
}
