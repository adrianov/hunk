#include "GitDetail.hpp"

#include <QDir>

namespace {

QString baseCommit(const QString &root, const QString &baseRef, const QString &headRef)
{
    const QString base = baseRef.trimmed();
    const QString head = headRef.trimmed().isEmpty() ? QStringLiteral("HEAD") : headRef.trimmed();
    if (base.isEmpty() || isBranchPoint(base))
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
    if (head.code != 0)
        return {};
    const GitCmd status = runGit(root, {QStringLiteral("status"), QStringLiteral("--porcelain"), QStringLiteral("-uno")});
    if (status.code != 0)
        return {};
    return head.out.trimmed() + QLatin1Char('\n') + baseCommit(root, baseRef, headRef) + QLatin1Char('\n') + status.out;
}
