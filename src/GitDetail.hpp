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
QStringList baseRefs(const QString &root);
QStringList baseChoices(const QString &root);
QStringList branchRefs(const QString &root, const QString &current);
QString branchPoint(const QString &root, const QString &headRef);
bool fillMerge(GitResult &result, QStringList *args);
