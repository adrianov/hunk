// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "Syntax.hpp"

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

bool hasText(const QList<SynSpan> &spans, SynKind kind, const QString &line, const QString &expect)
{
    for (const SynSpan &span : spans) {
        if (span.kind == kind && line.mid(span.start, span.end - span.start) == expect)
            return true;
    }
    return false;
}

DiffDoc colorFile(const QString &path, const QStringList &lines)
{
    FileDiff file;
    file.newPath = path;
    for (const QString &line : lines) {
        DiffRow row;
        row.rightText = line;
        row.leftText = line;
        file.rows.append(row);
    }
    DiffDoc doc;
    doc.files.append(file);
    highlightDoc(&doc);
    return doc;
}

DiffRow firstRow(const QString &path, const QString &line)
{
    return colorFile(path, {line}).files.at(0).rows.at(0);
}

bool rightHas(const DiffRow &row, SynKind kind, const QString &expect)
{
    return hasText(row.rightSyn, kind, row.rightText, expect);
}

void testRubyDef()
{
    const DiffRow row = firstRow("lib/app.rb", "def greet(name) # hi");
    CHECK(rightHas(row, SynKind::Keyword, "def"));
    CHECK(rightHas(row, SynKind::Method, "greet"));
    CHECK(rightHas(row, SynKind::Comment, "# hi"));
}

void testBareDef()
{
    CHECK(rightHas(firstRow("lib/app.rb", "def wave"), SynKind::Method, "wave"));
}

void testNotDef()
{
    CHECK(rightHas(firstRow("lib/app.rb", "mydef wave"), SynKind::Variable, "wave"));
}

void testScopeType()
{
    const DiffRow row = firstRow("src/app.cpp", "Foo::Bar");
    CHECK(rightHas(row, SynKind::Type, "Foo"));
    CHECK(rightHas(row, SynKind::Type, "Bar"));
}

void testRubyParam()
{
    CHECK(rightHas(firstRow("lib/app.rb", "def greet(name)"), SynKind::Variable, "name"));
}

void testRubyCall()
{
    const DiffRow row = firstRow("lib/app.rb", "amount = ask_rate.zero?");
    CHECK(rightHas(row, SynKind::Variable, "amount"));
    CHECK(rightHas(row, SynKind::Variable, "ask_rate"));
    CHECK(rightHas(row, SynKind::Method, "zero?"));
}

void testRubyValue()
{
    const DiffRow row = firstRow("lib/app.rb", "  @name = \"Ada\"");
    CHECK(rightHas(row, SynKind::Variable, "@name"));
    CHECK(rightHas(row, SynKind::String, "\"Ada\""));
}

void testRubyInterp()
{
    const DiffRow row = firstRow("lib/app.rb", "SQL = \"a >= #{MIN_STOCK}\"");
    CHECK(rightHas(row, SynKind::String, "\"a >= #{"));
    CHECK(rightHas(row, SynKind::Type, "MIN_STOCK"));
    CHECK(rightHas(row, SynKind::String, "}\""));
}

void testBlock()
{
    const QList<DiffRow> rows = colorFile(QStringLiteral("src/app.cpp"), {QStringLiteral("/* start"), QStringLiteral(" still"), QStringLiteral(" end */ int x;")}).files.at(0).rows;
    CHECK(hasText(rows.at(1).rightSyn, SynKind::Comment, rows.at(1).rightText, QStringLiteral(" still")));
    CHECK(hasText(rows.at(2).rightSyn, SynKind::Keyword, rows.at(2).rightText, QStringLiteral("int")));
}

void testCppName()
{
    CHECK(rightHas(firstRow("src/app.cpp", "int x;"), SynKind::Variable, "x"));
}

void testPlain()
{
    CHECK(firstRow("notes.md", "def not code").rightSyn.isEmpty());
}

void testApos()
{
    const DiffRow row = firstRow("lib/app.rb", "name = don't");
    CHECK(rightHas(row, SynKind::Variable, "name"));
    CHECK(!rightHas(row, SynKind::String, "'t"));
}

void testRename()
{
    FileDiff file;
    file.oldPath = QStringLiteral("app.rb");
    file.newPath = QStringLiteral("app.cpp");
    DiffRow row;
    row.leftText = QStringLiteral("def greet");
    row.rightText = QStringLiteral("int greet;");
    file.rows.append(row);
    DiffDoc doc;
    doc.files.append(file);
    highlightDoc(&doc);
    const DiffRow colored = doc.files.at(0).rows.at(0);
    CHECK(hasText(colored.leftSyn, SynKind::Keyword, colored.leftText, QStringLiteral("def")));
    CHECK(hasText(colored.rightSyn, SynKind::Keyword, colored.rightText, QStringLiteral("int")));
}

} // namespace

int mdTests();
int emphTests();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    testRubyDef();
    testBareDef();
    testNotDef();
    testScopeType();
    testRubyParam();
    testRubyCall();
    testRubyValue();
    testRubyInterp();
    testBlock();
    testCppName();
    testPlain();
    g_fails += mdTests();
    g_fails += emphTests();
    testApos();
    testRename();
    Q_UNUSED(app);
    return g_fails == 0 ? 0 : 1;
}
