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
    CHECK(rightHas(row, SynKind::Variable, "name"));
    CHECK(rightHas(row, SynKind::Comment, "# hi"));
}

void testBareDef()
{
    CHECK(rightHas(firstRow("lib/app.rb", "def wave"), SynKind::Method, "wave"));
    CHECK(rightHas(firstRow("lib/app.rb", "mydef wave"), SynKind::Variable, "wave"));
}

void testScopeType()
{
    const DiffRow row = firstRow("src/app.cpp", "Foo::Bar");
    CHECK(rightHas(row, SynKind::Type, "Foo"));
    CHECK(rightHas(row, SynKind::Type, "Bar"));
    CHECK(rightHas(firstRow("src/app.cpp", "int x;"), SynKind::Variable, "x"));
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

void testRubyRegex()
{
    const DiffRow row = firstRow("spec/a_spec.rb", "expect(io.scan(/id=\"([^\"]+)\"/).flatten).to eq(ids / 2 + rest / 3)");
    CHECK(rightHas(row, SynKind::String, "/id=\"([^\"]+)\"/"));
    CHECK(rightHas(row, SynKind::Variable, "rest"));
    CHECK(rightHas(firstRow("app.tsx", "<Foo /> x /= 2; <p>a</p>"), SynKind::Variable, "a"));
}

void testBlock()
{
    const QList<DiffRow> rows = colorFile("src/app.cpp", {"/* start", " still", " end */ int x;"}).files.at(0).rows;
    CHECK(rightHas(rows.at(1), SynKind::Comment, " still"));
    CHECK(rightHas(rows.at(2), SynKind::Keyword, "int"));
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
int envTests();

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    testRubyDef();
    testBareDef();
    testScopeType();
    testRubyCall();
    testRubyValue();
    testRubyInterp();
    testRubyRegex();
    testBlock();
    testPlain();
    g_fails += mdTests();
    g_fails += emphTests();
    testApos();
    g_fails += envTests();
    testRename();
    Q_UNUSED(app);
    return g_fails == 0 ? 0 : 1;
}
