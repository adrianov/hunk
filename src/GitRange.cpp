#include "GitDetail.hpp"

namespace {

QString shortRef(const QString &root, const QString &ref)
{
    return runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--short"), ref}).out.trimmed();
}

bool mergeBase(GitResult &result, const QString &left, const QString &headRef, QString *leftCommit, QString *leftShort)
{
    const GitCmd base = runGit(result.root, {QStringLiteral("merge-base"), left, headRef});
    if (base.code != 0) {
        result.error = base.err.isEmpty() ? QStringLiteral("Cannot find merge base") : base.err;
        return false;
    }
    *leftCommit = base.out.trimmed();
    *leftShort = shortRef(result.root, *leftCommit);
    return true;
}

QString resolveLeft(GitResult &result, const QString &headRef)
{
    if (result.baseRef != branchPointName())
        return result.baseRef;
    const QString point = branchPoint(result.root, headRef);
    if (point.isEmpty())
        result.error = QStringLiteral("Cannot find branching point");
    return point;
}

void setMergeLabels(GitResult &result, const QString &headRef, const QString &leftShort)
{
    result.leftLabel = leftShort;
    result.rightLabel = shortRef(result.root, headRef);
    result.title = headRef + QStringLiteral(" vs ") + result.baseRef;
}

// The checked-out branch includes the working tree. Another branch is commits only.
void addRange(QStringList *args, const GitResult &result, const QString &leftCommit, const QString &headRef)
{
    if (headRef == QLatin1String("HEAD") || headRef == result.branch)
        args->append(leftCommit);
    else
        args->append(leftCommit + QStringLiteral("...") + headRef);
}

} // namespace

bool fillMerge(GitResult &result, QStringList *args)
{
    if (result.baseRef.isEmpty()) {
        result.error = QStringLiteral("No base branch. Set upstream, or type main.");
        return false;
    }
    const QString headRef = result.headRef.isEmpty() ? QStringLiteral("HEAD") : result.headRef;
    const QString left = resolveLeft(result, headRef);
    if (left.isEmpty())
        return false;
    QString leftCommit;
    QString leftShort;
    if (!mergeBase(result, left, headRef, &leftCommit, &leftShort))
        return false;
    setMergeLabels(result, headRef, leftShort);
    addRange(args, result, leftCommit, headRef);
    return true;
}
