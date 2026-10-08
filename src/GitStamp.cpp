#include "GitDetail.hpp"

namespace {

QString baseCommit(const QString &root, const QString &baseRef, const QString &headRef)
{
    const QString base = baseRef.trimmed();
    const QString head = headRef.trimmed().isEmpty() ? QStringLiteral("HEAD") : headRef.trimmed();
    if (base.isEmpty() || base == branchPointName())
        return branchPoint(root, head);
    const GitCmd rev = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), base});
    return rev.code == 0 ? rev.out.trimmed() : QString();
}

} // namespace

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
