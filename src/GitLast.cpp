// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

namespace {

// First parent of HEAD. A root commit compares with the empty tree.
void setParent(GitResult &result)
{
    const GitCmd parent = runGit(result.root, {QStringLiteral("rev-parse"), QStringLiteral("--short"),
                                               QStringLiteral("--verify"), QStringLiteral("--quiet"), QStringLiteral("HEAD^")});
    if (parent.code == 0) {
        result.leftRev = parent.out.trimmed();
        result.leftLabel = result.leftRev;
        return;
    }
    result.leftRev = runGit(result.root, {QStringLiteral("hash-object"), QStringLiteral("-t"), QStringLiteral("tree"),
                                          QStringLiteral("/dev/null")}).out.trimmed();
    result.leftLabel = QStringLiteral("Empty");
}

} // namespace

void fillLast(GitResult &result, const QString &headShort, QStringList *args)
{
    setParent(result);
    result.rightRev = headShort;
    result.rightLabel = headShort;
    result.title = result.branch + QStringLiteral(" last commit ") + headShort;
    *args << result.leftRev << headShort;
}
