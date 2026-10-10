// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitRepo.hpp"

#include "DiffParse.hpp"
#include "GitDetail.hpp"

namespace {

bool renameSplit(const DiffDoc &doc, const QString &oldPath, const QString &newPath)
{
    bool removed = false;
    bool added = false;
    for (const FileDiff &file : doc.files) {
        if (file.removed && file.path() == oldPath)
            removed = true;
        if (file.added && file.path() == newPath)
            added = true;
    }
    return removed && added;
}

QStringList cachedArgs()
{
    return {QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("--name-status"),
            QStringLiteral("-z"), QStringLiteral("--find-renames"), QStringLiteral("--diff-filter=R")};
}

// Empty rightRev is the worktree. ":" is the index, so a staged diff stays off the worktree file.
QStringList pairArgs(const QString &leftRev, const QString &rightRev, const QString &oldPath, const QString &newPath)
{
    QStringList args{QStringLiteral("diff"), QStringLiteral("-w"), QStringLiteral("-W"), QStringLiteral("--no-prefix"),
                     QStringLiteral("--diff-algorithm=histogram"), leftRev + QLatin1Char(':') + oldPath};
    if (rightRev == QLatin1String(":"))
        args << QLatin1Char(':') + newPath;
    else
        args << QStringLiteral("--") << newPath;
    return args;
}

void joinOne(GitResult *result, const QString &status, const QString &oldPath, const QString &newPath)
{
    if (!status.startsWith(QLatin1Char('R')) || !renameSplit(result->doc, oldPath, newPath))
        return;
    const GitCmd diff = runGit(result->root, pairArgs(result->leftRev, result->rightRev, oldPath, newPath));
    if (diff.code == 0)
        joinRename(&result->doc, oldPath, newPath, diff.out);
}

void joinListed(GitResult *result, const QString &out)
{
    const QStringList parts = out.split(QLatin1Char('\0'), Qt::SkipEmptyParts);
    for (int index = 0; index + 2 < parts.size(); index += 3)
        joinOne(result, parts.at(index), parts.at(index + 1), parts.at(index + 2));
}

bool joinable(const GitResult &result)
{
    if (!result.error.isEmpty() || result.leftRev.isEmpty())
        return false;
    return result.rightRev.isEmpty() || result.rightRev == QLatin1String(":");
}

} // namespace

void joinIndexRenames(GitResult *result)
{
    if (!joinable(*result))
        return;
    const GitCmd names = runGit(result->root, cachedArgs());
    if (names.code != 0)
        return;
    joinListed(result, names.out);
}
