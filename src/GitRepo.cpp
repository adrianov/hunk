#include "GitRepo.hpp"

#include "GitDetail.hpp"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent>

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
        result.bases = baseChoices(result.root);
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
    return true;
}

void pickRefs(GitResult &result, const QString &baseRef, const QString &headRef)
{
    result.bases = baseChoices(result.root);
    result.branches = branchRefs(result.root, result.branch);
    result.headRef = headRef.trimmed().isEmpty() ? result.branch : headRef.trimmed();
    result.baseRef = baseRef.trimmed().isEmpty() ? branchPointName() : baseRef.trimmed();
}

GitResult loadGit(const QString &startPath, DiffMode mode, const QString &baseRef, const QString &headRef)
{
    GitResult result;
    QString cwd;
    if (!readCwd(result, startPath, &cwd) || !readRoot(result, cwd))
        return result;
    readBranch(result);
    QString headShort;
    if (!readHead(result, &headShort))
        return result;
    pickRefs(result, baseRef, headRef);
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

void GitRepo::load(const QString &startPath, DiffMode mode, const QString &baseRef, const QString &headRef)
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
    watcher->setFuture(QtConcurrent::run([startPath, mode, baseRef, headRef]() {
        return loadGit(startPath, mode, baseRef, headRef);
    }));
}
