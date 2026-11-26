// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "GitRepo.hpp"

#include <QList>
#include <QString>
#include <QStringList>

struct GitCmd {
    int code = -1;
    QString out;
    QString err;
};

GitCmd runGit(const QString &cwd, const QStringList &args);
QString branchPointName();
QString parentRef(const QString &root);
QString branchPointLabel(const QString &root, const QString &headRef);
QString forkLabel(const QString &root, const QString &headRef);
bool isBranchPoint(const QString &root, const QString &headRef, const QString &ref);
QStringList baseRefs(const QString &root);
QStringList baseChoices(const QString &root, const QString &headRef);
QStringList branchRefs(const QString &root, const QString &current);
QString branchPoint(const QString &root, const QString &headRef);
QString refState(const QString &root);
QString mainStamp(const QString &root);
QString branchDrift(const QString &root);
QStringList ignoredDirs(const QString &root);
QString stampKey(DiffMode mode, const QString &baseRef, const QString &headRef, const QString &disk);
bool reuseDiff(GitResult &result, DiffMode mode, const QString &baseRef, const QString &headRef, const QString &previous,
               bool quiet);
void addUntracked(GitResult *result);
bool fillMerge(GitResult &result, QStringList *args);
void fillLast(GitResult &result, const QString &headShort, QStringList *args);
void fillDocGaps(DiffDoc *doc, const GitResult &result);
// Tracked and untracked worktree files whose text contains `query`. Binary files are skipped.
QStringList grepFiles(const QString &root, const QString &query);
// Tracked and untracked files directly under `folder` (empty for the repository root).
QStringList dirFiles(const QString &root, const QString &folder);
// Those paths as opened unchanged FileDiffs. Reuses `prior` entries whose worktree mtime is unchanged.
QList<FileDiff> dirPlain(const QString &root, const QString &folder, const QList<FileDiff> &prior = {});
