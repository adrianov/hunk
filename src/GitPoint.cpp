// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

namespace {

bool parentSuffix(const QString &parent, const QString &name)
{
    return name == parent || name.startsWith(parent + QLatin1Char('~')) || name.startsWith(parent + QLatin1Char('^'));
}

QString relativeName(const QString &parent, const QString &named)
{
    if (parentSuffix(parent, named))
        return named;
    const QString remote = QStringLiteral("remotes/") + parent;
    if (!parentSuffix(remote, named))
        return {};
    return named.mid(int(QStringLiteral("remotes/").size()));
}

QString peelFirst(QString name)
{
    const int mark = name.lastIndexOf(QLatin1Char('~'));
    if (mark < 0 || name.mid(mark) != QLatin1String("~1"))
        return name;
    if (mark > 0 && name.at(mark - 1).isDigit())
        return name;
    name.replace(mark, 2, QLatin1Char('^'));
    return name;
}

QString namedPoint(const QString &root, const QString &parent, const QString &point)
{
    const QString full = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--symbolic-full-name"), parent}).out.trimmed();
    if (full.isEmpty())
        return parent;
    const GitCmd named = runGit(root, {QStringLiteral("name-rev"), QStringLiteral("--name-only"), QStringLiteral("--no-undefined"),
                                        QStringLiteral("--refs=") + full, point});
    if (named.code != 0)
        return parent;
    const QString relative = relativeName(parent, named.out.trimmed());
    return relative.isEmpty() ? parent : peelFirst(relative);
}

bool sameTip(const QString &root, const QString &parent, const QString &headRef)
{
    const QString point = branchPoint(root, headRef);
    if (point.isEmpty())
        return false;
    const GitCmd tip = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), parent});
    return tip.code == 0 && tip.out.trimmed() == point;
}

} // namespace

QString branchPointLabel(const QString &root, const QString &headRef)
{
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
