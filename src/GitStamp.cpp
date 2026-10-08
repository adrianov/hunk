// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

#include <QDir>

namespace {

QString baseCommit(const QString &root, const QString &baseRef, const QString &headRef)
{
    const QString base = baseRef.trimmed();
    const QString head = headRef.trimmed().isEmpty() ? QStringLiteral("HEAD") : headRef.trimmed();
    if (base.isEmpty() || isBranchPoint(root, head, base))
        return branchPoint(root, head);
    const GitCmd rev = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), base});
    return rev.code == 0 ? rev.out.trimmed() : QString();
}

void takeIgnored(QStringList *dirs, const QString &root, QString row)
{
    if (!row.endsWith(QLatin1Char('/')))
        return;
    row.chop(1);
    dirs->append(QDir::cleanPath(QDir(root).absoluteFilePath(row)));
}

QString joinStamp(const QString &head, const QString &base, const QString &status, const QString &root)
{
    return head + QLatin1Char('\n') + base + QLatin1Char('\n') + status + QLatin1Char('\n') + mainStamp(root);
}

} // namespace

QStringList ignoredDirs(const QString &root)
{
    QStringList dirs;
    const GitCmd listed = runGit(root, {QStringLiteral("ls-files"), QStringLiteral("-o"), QStringLiteral("-i"),
                                        QStringLiteral("--directory"), QStringLiteral("--exclude-standard"),
                                        QStringLiteral("-z")});
    if (listed.code != 0)
        return dirs;
    for (QString row : listed.out.split(QChar(u'\0'), Qt::SkipEmptyParts))
        takeIgnored(&dirs, root, row);
    return dirs;
}

QString workStamp(const QString &root, const QString &baseRef, const QString &headRef)
{
    const GitCmd head = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
    const GitCmd status = runGit(root, {QStringLiteral("status"), QStringLiteral("--porcelain"), QStringLiteral("-uno")});
    if (head.code != 0 || status.code != 0)
        return {};
    return joinStamp(head.out.trimmed(), baseCommit(root, baseRef, headRef), status.out, root);
}
