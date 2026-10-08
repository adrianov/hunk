// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "Syntax.hpp"

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

void testMdHead()
{
    const DiffRow row = colorFile(QStringLiteral("README.md"), {QStringLiteral("# Hello")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("#")));
}

void testMdCode()
{
    const DiffRow row = colorFile(QStringLiteral("README.md"), {QStringLiteral("see `code`")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::String, row.rightText, QStringLiteral("`code`")));
}

void testMdLink()
{
    const DiffRow row = colorFile(QStringLiteral("notes.md"), {QStringLiteral("[Hunk](https://example.com)")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Method, row.rightText, QStringLiteral("Hunk")));
    CHECK(hasText(row.rightSyn, SynKind::String, row.rightText, QStringLiteral("https://example.com")));
}

void testMdFence()
{
    const QList<DiffRow> rows = colorFile(QStringLiteral("doc.markdown"), {
        QStringLiteral("```"), QStringLiteral("int x;"), QStringLiteral("```")
    }).files.at(0).rows;
    CHECK(hasText(rows.at(0).rightSyn, SynKind::Keyword, rows.at(0).rightText, QStringLiteral("```")));
    CHECK(hasText(rows.at(2).rightSyn, SynKind::Keyword, rows.at(2).rightText, QStringLiteral("```")));
}

void testMdBlock()
{
    const DiffRow row = colorFile(QStringLiteral("doc.markdown"), {
        QStringLiteral("```"), QStringLiteral("int x;"), QStringLiteral("```")
    }).files.at(0).rows.at(1);
    CHECK(hasText(row.rightSyn, SynKind::String, row.rightText, QStringLiteral("int x;")));
    CHECK(!hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("int")));
}

} // namespace

int mdTests()
{
    testMdHead();
    testMdCode();
    testMdLink();
    testMdFence();
    testMdBlock();
    return g_fails;
}
