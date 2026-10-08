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

void testRubyDef()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("def greet(name) # hi")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("def")));
    CHECK(hasText(row.rightSyn, SynKind::Method, row.rightText, QStringLiteral("greet")));
    CHECK(hasText(row.rightSyn, SynKind::Comment, row.rightText, QStringLiteral("# hi")));
}

void testBareDef()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("def wave")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Method, row.rightText, QStringLiteral("wave")));
}

void testNotDef()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("mydef wave")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("wave")));
}

void testScopeType()
{
    const DiffRow row = colorFile(QStringLiteral("src/app.cpp"), {QStringLiteral("Foo::Bar")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Type, row.rightText, QStringLiteral("Foo")));
    CHECK(hasText(row.rightSyn, SynKind::Type, row.rightText, QStringLiteral("Bar")));
}

void testRubyParam()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("def greet(name)")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("name")));
}

void testRubyCall()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("amount = ask_rate.zero?")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("amount")));
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("ask_rate")));
    CHECK(hasText(row.rightSyn, SynKind::Method, row.rightText, QStringLiteral("zero?")));
}

void testRubyValue()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("  @name = \"Ada\"")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("@name")));
    CHECK(hasText(row.rightSyn, SynKind::String, row.rightText, QStringLiteral("\"Ada\"")));
}

void testBlock()
{
    const QList<DiffRow> rows = colorFile(QStringLiteral("src/app.cpp"), {QStringLiteral("/* start"), QStringLiteral(" still"), QStringLiteral(" end */ int x;")}).files.at(0).rows;
    CHECK(hasText(rows.at(1).rightSyn, SynKind::Comment, rows.at(1).rightText, QStringLiteral(" still")));
    CHECK(hasText(rows.at(2).rightSyn, SynKind::Keyword, rows.at(2).rightText, QStringLiteral("int")));
}

void testCppName()
{
    const DiffRow row = colorFile(QStringLiteral("src/app.cpp"), {QStringLiteral("int x;")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("x")));
}

void testPlain()
{
    CHECK(colorFile(QStringLiteral("notes.md"), {QStringLiteral("def not code")}).files.at(0).rows.at(0).rightSyn.isEmpty());
}

void testApos()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("name = don't")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Variable, row.rightText, QStringLiteral("name")));
    CHECK(!hasText(row.rightSyn, SynKind::String, row.rightText, QStringLiteral("'t")));
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
    testBlock();
    testCppName();
    testPlain();
    testApos();
    testRename();
    Q_UNUSED(app);
    return g_fails == 0 ? 0 : 1;
}
