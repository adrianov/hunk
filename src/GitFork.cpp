// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

#include <QHash>

namespace {

QString branchOf(const QString &root, const QString &head)
{
    if (head != QLatin1String("HEAD"))
        return head;
    return runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("HEAD")}).out.trimmed();
}

bool ownRef(const QString &full, const QString &head)
{
    const QString tail = QLatin1Char('/') + head;
    return full == QLatin1String("refs/heads/") + head
        || (full.startsWith(QLatin1String("refs/remotes/")) && full.endsWith(tail));
}

bool localRef(const QString &full)
{
    return full.startsWith(QLatin1String("refs/heads/"));
}

bool betterTip(const QString &have, const QString &next)
{
    if (localRef(next) != localRef(have))
        return localRef(next);
    const QString origin = QStringLiteral("refs/remotes/origin/");
    return next.startsWith(origin) && !have.startsWith(origin);
}

QString tipName(const QString &full)
{
    const QString heads = QStringLiteral("refs/heads/");
    const QString remotes = QStringLiteral("refs/remotes/");
    if (full.startsWith(heads))
        return full.mid(heads.size());
    if (full.startsWith(remotes))
        return full.mid(remotes.size());
    return full;
}

void noteTip(QHash<QString, QString> *tips, const QString &line, const QString &head)
{
    const int space = line.indexOf(QLatin1Char(' '));
    if (space <= 0)
        return;
    const QString ref = line.mid(space + 1);
    if (ownRef(ref, head))
        return;
    const QString sha = line.left(space);
    const QString have = tips->value(sha);
    if (have.isEmpty() || betterTip(have, ref))
        tips->insert(sha, ref);
}

QHash<QString, QString> branchTips(const QString &root, const QString &head)
{
    const GitCmd listed = runGit(root, {QStringLiteral("for-each-ref"),
                                        QStringLiteral("--format=%(objectname) %(refname)"),
                                        QStringLiteral("refs/heads"), QStringLiteral("refs/remotes")});
    QHash<QString, QString> tips;
    for (const QString &line : listed.out.split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        noteTip(&tips, line, head);
    return tips;
}

QString scanTips(const QString &root, const QString &head, const QString &point, const QHash<QString, QString> &tips)
{
    const GitCmd list = runGit(root, {QStringLiteral("rev-list"), QStringLiteral("--first-parent"),
                                      point + QStringLiteral("..") + head});
    if (list.code != 0)
        return {};
    for (const QString &line : list.out.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QString ref = tips.value(line.trimmed());
        if (!ref.isEmpty())
            return tipName(ref);
    }
    return {};
}

} // namespace

QString forkLabel(const QString &root, const QString &headRef)
{
    const QString parent = parentRef(root);
    if (parent.isEmpty())
        return {};
    const QString head = headRef.trimmed().isEmpty() ? QStringLiteral("HEAD") : headRef.trimmed();
    const GitCmd base = runGit(root, {QStringLiteral("merge-base"), parent, head});
    if (base.code != 0)
        return {};
    return scanTips(root, head, base.out.trimmed(), branchTips(root, branchOf(root, head)));
}
