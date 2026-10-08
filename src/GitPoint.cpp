// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

namespace {

bool parentSuffix(const QString &parent, const QString &name)
{
    return name == parent || name.startsWith(parent + QLatin1Char('~')) || name.startsWith(parent + QLatin1Char('^'));
}

bool sameCommit(const QString &root, const QString &name, const QString &point)
{
    const GitCmd tip = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), name});
    return tip.code == 0 && tip.out.trimmed() == point;
}

QString stepName(const QString &parent, int steps)
{
    if (steps <= 0)
        return parent;
    if (steps == 1)
        return parent + QStringLiteral("^");
    return parent + QLatin1Char('~') + QString::number(steps);
}

int parentSteps(const QString &root, const QString &parent, const QString &point)
{
    const GitCmd count = runGit(root, {QStringLiteral("rev-list"), QStringLiteral("--first-parent"), QStringLiteral("--count"),
                                        point + QStringLiteral("..") + parent});
    return count.code == 0 ? count.out.trimmed().toInt() : -1;
}

QString namedPoint(const QString &root, const QString &parent, const QString &point)
{
    if (sameCommit(root, parent, point))
        return parent;
    const int steps = parentSteps(root, parent, point);
    const QString name = steps < 0 ? parent : stepName(parent, steps);
    return sameCommit(root, name, point) ? name : parent;
}

bool sameTip(const QString &root, const QString &parent, const QString &headRef)
{
    const QString point = branchPoint(root, headRef);
    return !point.isEmpty() && sameCommit(root, parent, point);
}

} // namespace

QString branchPointLabel(const QString &root, const QString &headRef)
{
    const QString cut = forkLabel(root, headRef);
    if (!cut.isEmpty())
        return cut;
    const QString parent = parentRef(root);
    if (parent.isEmpty())
        return branchPointName();
    const QString head = headRef.trimmed().isEmpty() ? QStringLiteral("HEAD") : headRef.trimmed();
    const QString point = branchPoint(root, head);
    return point.isEmpty() ? parent : namedPoint(root, parent, point);
}

bool isBranchPoint(const QString &root, const QString &headRef, const QString &ref)
{
    const QString text = ref.trimmed();
    if (text == branchPointName() || text.startsWith(branchPointName() + QStringLiteral(" (")))
        return true;
    const QString parent = parentRef(root);
    const QString head = headRef.trimmed().isEmpty() ? QStringLiteral("HEAD") : headRef.trimmed();
    if (parentSuffix(parent, text) && text != parent)
        return true;
    return text == parent && sameTip(root, parent, head);
}
