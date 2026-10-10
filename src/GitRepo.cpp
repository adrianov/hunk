// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitRepo.hpp"

#include "DiffParse.hpp"
#include "GitDetail.hpp"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent>

void joinIndexRenames(GitResult *result);

namespace {

bool readCwd(GitResult &result, const QString &startPath, QString *cwd)
{
    const QFileInfo info(QDir::cleanPath(startPath));
    if (!info.exists()) {
        result.error = QStringLiteral("Path does not exist");
        return false;
    }
    *cwd = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    return true;
}

bool readRoot(GitResult &result, const QString &cwd)
{
    const GitCmd top = runGit(cwd, {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")});
    if (top.code != 0) {
        result.error = top.err.isEmpty() ? QStringLiteral("Not a git repository") : top.err;
        return false;
    }
    result.root = QDir::cleanPath(top.out.trimmed());
    return true;
}

void readBranch(GitResult &result)
{
    const GitCmd branch = runGit(result.root, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("HEAD")});
    result.branch = branch.code == 0 ? branch.out.trimmed() : QStringLiteral("HEAD");
    if (result.branch.isEmpty())
        result.branch = QStringLiteral("HEAD");
}

bool readHead(GitResult &result, QString *headShort)
{
    const GitCmd head = runGit(result.root, {QStringLiteral("rev-parse"), QStringLiteral("--short"), QStringLiteral("HEAD")});
    if (head.code != 0) {
        result.error = head.err.isEmpty() ? QStringLiteral("No commits yet") : head.err;
        result.bases = baseChoices(result.root, result.branch);
        result.branches = branchRefs(result.root, result.branch);
        return false;
    }
    *headShort = head.out.trimmed();
    return true;
}

QStringList diffFlags()
{
    return {QStringLiteral("diff"), QStringLiteral("-w"), QStringLiteral("-W"),
            QStringLiteral("--no-prefix"), QStringLiteral("--find-renames"),
            QStringLiteral("--diff-algorithm=histogram")};
}

void fillWorktree(GitResult &result, const QString &headShort, bool staged, QStringList *args)
{
    result.leftLabel = headShort;
    result.leftRev = QStringLiteral("HEAD");
    result.rightRev = staged ? QStringLiteral(":") : QString();
    if (staged) {
        result.rightLabel = QStringLiteral("Index");
        result.title = result.branch + QStringLiteral(" staged");
        *args << QStringLiteral("--cached");
        return;
    }
    result.rightLabel = QStringLiteral("Worktree");
    result.title = result.branch + QStringLiteral(" uncommitted");
    *args << QStringLiteral("HEAD");
}

bool applyMode(GitResult &result, DiffMode mode, const QString &headShort, QStringList *args)
{
    if (mode != DiffMode::MergeRequest) {
        fillWorktree(result, headShort, mode == DiffMode::Staged, args);
        return true;
    }
    return fillMerge(result, args);
}

bool runDiff(GitResult &result, const QStringList &args)
{
    const GitCmd diff = runGit(result.root, args);
    if (diff.code != 0) {
        result.error = diff.err.isEmpty() ? QStringLiteral("git diff failed") : diff.err;
        return false;
    }
    result.diffText = diff.out;
    result.doc = parseDiff(result.diffText);
    joinIndexRenames(&result);
    fillDocGaps(&result.doc, result);
    return true;
}

QString stampKey(DiffMode mode, const QString &baseRef, const QString &headRef, const QString &disk)
{
    return QString::number(static_cast<int>(mode)) + QLatin1Char('\n') + baseRef + QLatin1Char('\n') + headRef
        + QLatin1Char('\n') + disk;
}

bool reuseDiff(GitResult &result, DiffMode mode, const QString &baseRef, const QString &headRef, const QString &previous,
               bool quiet)
{
    result.diskStamp = workStamp(result.root, baseRef, headRef);
    result.stamp = stampKey(mode, baseRef.trimmed(), headRef.trimmed(), result.diskStamp);
    if (!quiet)
        return false;
    if (result.diskStamp.isEmpty())
        return true;
    return !previous.isEmpty() && result.stamp == previous;
}

void fillIgnored(GitResult &result)
{
    result.ignored = ignoredDirs(result.root);
    result.ignoredReady = true;
}

void pickRefs(GitResult &result, const QString &baseRef, const QString &headRef)
{
    result.drift = branchDrift(result.root);
    result.headRef = headRef.trimmed().isEmpty() ? result.branch : headRef.trimmed();
    result.bases = baseChoices(result.root, result.headRef);
    result.branches = branchRefs(result.root, result.branch);
    const QString base = baseRef.trimmed();
    result.baseRef = base.isEmpty() || isBranchPoint(result.root, result.headRef, base) ? result.bases.value(0) : base;
}

GitResult loadGit(const QString &startPath, DiffMode mode, const QString &baseRef, const QString &headRef,
                   const QString &previous, bool quiet, bool listIgnored)
{
    GitResult result;
    QString cwd;
    if (!readCwd(result, startPath, &cwd) || !readRoot(result, cwd))
        return result;
    if (listIgnored)
        fillIgnored(result);
    readBranch(result);
    QString headShort;
    if (!readHead(result, &headShort))
        return result;
    if (reuseDiff(result, mode, baseRef, headRef, previous, quiet)) {
        result.unchanged = true;
        return result;
    }
    pickRefs(result, baseRef, headRef);
    result.stamp = stampKey(mode, result.baseRef, result.headRef, result.diskStamp);
    QStringList args = diffFlags();
    if (!applyMode(result, mode, headShort, &args))
        return result;
    runDiff(result, args);
    return result;
}

} // namespace

GitRepo::GitRepo(QObject *parent)
    : QObject(parent)
{
}

void GitRepo::load(const QString &startPath, DiffMode mode, const QString &baseRef, const QString &headRef,
                    const QString &stamp, bool quiet, bool listIgnored)
{
    const int token = ++generation;
    auto *watcher = new QFutureWatcher<GitResult>(this);
    connect(watcher, &QFutureWatcher<GitResult>::finished, this, [this, watcher, token]() {
        const GitResult result = watcher->result();
        watcher->deleteLater();
        if (token != generation)
            return;
        emit ready(result);
    });
    watcher->setFuture(QtConcurrent::run([startPath, mode, baseRef, headRef, stamp, quiet, listIgnored]() {
        return loadGit(startPath, mode, baseRef, headRef, stamp, quiet, listIgnored);
    }));
}
