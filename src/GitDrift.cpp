// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

#include <QMutex>
#include <QMutexLocker>

namespace {

QMutex g_lock;

struct PairCache {
    QString root;
    QString state;
    QString local;
    QString remote;
    bool filled = false;
};

PairCache g_pair;

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

bool cachedPair(const QString &root, const QString &state, QString *local, QString *remote)
{
    const QMutexLocker lock(&g_lock);
    if (!g_pair.filled || g_pair.root != root || g_pair.state != state)
        return false;
    *local = g_pair.local;
    *remote = g_pair.remote;
    return true;
}

void storePair(const QString &root, const QString &state, const QString &local, const QString &remote)
{
    const QMutexLocker lock(&g_lock);
    g_pair = {root, state, local, remote, true};
}

// Branch names stay until refs or packed-refs change. Tips are read on every stamp.
bool takePair(const QString &root, QString *local, QString *remote)
{
    const QString state = refState(root);
    if (cachedPair(root, state, local, remote))
        return !local->isEmpty();
    const QString name = localMain(root, remote);
    storePair(root, state, name, *remote);
    *local = name;
    return !name.isEmpty();
}

QString bothTips(const QString &root, const QString &local, const QString &remote)
{
    const GitCmd tips = runGit(root, {QStringLiteral("rev-parse"), local, remote});
    const QStringList lines = tips.out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (tips.code != 0 || lines.size() < 2)
        return {};
    return lines.at(0).trimmed() + QLatin1Char(' ') + lines.at(1).trimmed();
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
    QString local;
    QString remote;
    if (!takePair(root, &local, &remote))
        return {};
    return bothTips(root, local, remote);
}

QString branchDrift(const QString &root)
{
    QString local;
    QString remote;
    if (!takePair(root, &local, &remote))
        return {};
    return countDrift(root, local, remote);
}
