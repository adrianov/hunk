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

DiffRow rowOf(const QString &path, const QString &line)
{
    FileDiff file;
    file.newPath = path;
    DiffRow row;
    row.rightText = line;
    file.rows.append(row);
    DiffDoc doc;
    doc.files.append(file);
    highlightDoc(&doc);
    return doc.files.at(0).rows.at(0);
}

bool has(const DiffRow &row, SynKind kind, const QString &expect)
{
    for (const SynSpan &span : row.rightSyn) {
        if (span.kind == kind && row.rightText.mid(span.start, span.end - span.start) == expect)
            return true;
    }
    return false;
}

void testEnv()
{
    const DiffRow row = rowOf(".env.test", "MANZANA_URL=http://manzana-api # local");
    CHECK(has(row, SynKind::Type, "MANZANA_URL"));
    CHECK(has(row, SynKind::String, "http://manzana-api"));
    CHECK(has(row, SynKind::Comment, "# local"));
    CHECK(has(rowOf(".env", "# Manzana API"), SynKind::Comment, "# Manzana API"));
    CHECK(rowOf(".env", "DIXY_API_TOKEN=token").rightSyn.size() == 2);
}

void testEnvExport()
{
    const DiffRow row = rowOf("config/app.env", "export TOKENS='a b'");
    CHECK(has(row, SynKind::Keyword, "export"));
    CHECK(has(row, SynKind::Type, "TOKENS"));
    CHECK(has(row, SynKind::String, "'a b'"));
}

} // namespace

int envTests()
{
    testEnv();
    testEnvExport();
    return g_fails;
}
