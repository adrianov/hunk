// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "GitRepo.hpp"

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
QString workStamp(const QString &root, const QString &baseRef, const QString &headRef);
QStringList ignoredDirs(const QString &root);
bool fillMerge(GitResult &result, QStringList *args);
void fillDocGaps(DiffDoc *doc, const GitResult &result);
