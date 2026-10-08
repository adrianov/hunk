// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

namespace {

bool hasRef(const QString &root, const QString &name)
{
    return runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), name}).code == 0;
}

QString originShort(const QString &root)
{
    const GitCmd origin = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("origin/HEAD")});
    const QString name = origin.out.trimmed();
    if (origin.code != 0 || !name.startsWith(QStringLiteral("origin/")))
        return {};
    return name.mid(int(QStringLiteral("origin/").size()));
}

QString paired(const QString &root, const QString &name, QString *remote)
{
    const QString remoteName = QStringLiteral("origin/") + name;
    if (name.isEmpty() || !hasRef(root, name) || !hasRef(root, remoteName))
        return {};
    *remote = remoteName;
    return name;
}

QString localMain(const QString &root, QString *remote)
{
    const QString preferred = paired(root, originShort(root), remote);
    if (!preferred.isEmpty())
        return preferred;
    const QString main = paired(root, QStringLiteral("main"), remote);
    return main.isEmpty() ? paired(root, QStringLiteral("master"), remote) : main;
}

QString driftText(const QString &local, const QString &remote, int ahead, int behind)
{
    if (ahead > 0 && behind > 0)
        return QStringLiteral("%1 is %2 ahead and %3 behind %4").arg(local).arg(ahead).arg(behind).arg(remote);
    if (ahead > 0)
        return QStringLiteral("%1 is %2 ahead of %3").arg(local).arg(ahead).arg(remote);
    if (behind > 0)
        return QStringLiteral("%1 is %2 behind %3").arg(local).arg(behind).arg(remote);
    return {};
}

QString tipOf(const QString &root, const QString &name)
{
    return runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), name}).out.trimmed();
}

QString countDrift(const QString &root, const QString &local, const QString &remote)
{
    const GitCmd counts = runGit(root, {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"),
                                         local + QStringLiteral("...") + remote});
    const QStringList parts = counts.out.trimmed().split(QLatin1Char('\t'));
    if (counts.code != 0 || parts.size() != 2)
        return {};
    return driftText(local, remote, parts.at(0).toInt(), parts.at(1).toInt());
}

} // namespace

QString mainStamp(const QString &root)
{
    QString remote;
    const QString local = localMain(root, &remote);
    if (local.isEmpty())
        return {};
    return tipOf(root, local) + QLatin1Char(' ') + tipOf(root, remote);
}

QString branchDrift(const QString &root)
{
    QString remote;
    const QString local = localMain(root, &remote);
    return local.isEmpty() ? QString() : countDrift(root, local, remote);
}
