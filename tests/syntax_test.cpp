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
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("def greet # hi")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("def")));
    CHECK(hasText(row.rightSyn, SynKind::Comment, row.rightText, QStringLiteral("# hi")));
}

void testRubyValue()
{
    const DiffRow row = colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("  @name = \"Ada\"")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Type, row.rightText, QStringLiteral("@name")));
    CHECK(hasText(row.rightSyn, SynKind::String, row.rightText, QStringLiteral("\"Ada\"")));
}

void testBlock()
{
    const QList<DiffRow> rows = colorFile(QStringLiteral("src/app.cpp"), {QStringLiteral("/* start"), QStringLiteral(" still"), QStringLiteral(" end */ int x;")}).files.at(0).rows;
    CHECK(hasText(rows.at(1).rightSyn, SynKind::Comment, rows.at(1).rightText, QStringLiteral(" still")));
    CHECK(hasText(rows.at(2).rightSyn, SynKind::Keyword, rows.at(2).rightText, QStringLiteral("int")));
}

void testPlain()
{
    CHECK(colorFile(QStringLiteral("notes.md"), {QStringLiteral("def not code")}).files.at(0).rows.at(0).rightSyn.isEmpty());
}

void testApos()
{
    CHECK(colorFile(QStringLiteral("lib/app.rb"), {QStringLiteral("name = don't")}).files.at(0).rows.at(0).rightSyn.isEmpty());
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
    testRubyValue();
    testBlock();
    testPlain();
    testApos();
    testRename();
    Q_UNUSED(app);
    return g_fails == 0 ? 0 : 1;
}
