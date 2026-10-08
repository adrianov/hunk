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

void testMdEmph()
{
    const DiffRow row = colorFile(QStringLiteral("README.md"), {
        QStringLiteral("see **bold** and *italics*")
    }).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Strong, row.rightText, QStringLiteral("bold")));
    CHECK(hasText(row.rightSyn, SynKind::Emph, row.rightText, QStringLiteral("italics")));
    CHECK(hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("**")));
}

void testMdStrike()
{
    const DiffRow row = colorFile(QStringLiteral("README.md"), {QStringLiteral("~~gone~~")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Strike, row.rightText, QStringLiteral("gone")));
    CHECK(hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("~~")));
}

void testMdUnder()
{
    const DiffRow row = colorFile(QStringLiteral("README.md"), {QStringLiteral("__bold__ and _word_")}).files.at(0).rows.at(0);
    CHECK(hasText(row.rightSyn, SynKind::Strong, row.rightText, QStringLiteral("bold")));
    CHECK(hasText(row.rightSyn, SynKind::Emph, row.rightText, QStringLiteral("word")));
}

void testMdSnake()
{
    const DiffRow row = colorFile(QStringLiteral("README.md"), {QStringLiteral("a_b_c and 2 * 3")}).files.at(0).rows.at(0);
    CHECK(!hasText(row.rightSyn, SynKind::Emph, row.rightText, QStringLiteral("b")));
    CHECK(!hasText(row.rightSyn, SynKind::Keyword, row.rightText, QStringLiteral("*")));
}

} // namespace

int emphTests()
{
    testMdEmph();
    testMdStrike();
    testMdUnder();
    testMdSnake();
    return g_fails;
}
