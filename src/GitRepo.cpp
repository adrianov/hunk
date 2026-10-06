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
        result.bases = baseRefs(result.root);
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

bool mergeBase(GitResult &result, QString *leftShort)
{
    const GitCmd base = runGit(result.root, {QStringLiteral("merge-base"), result.baseRef, QStringLiteral("HEAD")});
    if (base.code != 0) {
        result.error = base.err.isEmpty() ? QStringLiteral("Cannot find merge base") : base.err;
        return false;
    }
    *leftShort = runGit(result.root, {QStringLiteral("rev-parse"), QStringLiteral("--short"), base.out.trimmed()}).out.trimmed();
    return true;
}

bool fillMerge(GitResult &result, const QString &headShort, QStringList *args)
{
    if (result.baseRef.isEmpty()) {
        result.error = QStringLiteral("No base branch. Set upstream, or type main.");
        return false;
    }
    QString leftShort;
    if (!mergeBase(result, &leftShort))
        return false;
    result.leftLabel = leftShort;
    result.rightLabel = headShort;
    result.title = result.branch + QStringLiteral(" vs ") + result.baseRef;
    *args << (result.baseRef + QStringLiteral("...HEAD"));
    return true;
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
    return fillMerge(result, headShort, args);
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

GitResult loadGit(const QString &startPath, DiffMode mode, const QString &baseRef)
{
    GitResult result;
    QString cwd;
    if (!readCwd(result, startPath, &cwd) || !readRoot(result, cwd))
        return result;
    readBranch(result);
    QString headShort;
    if (!readHead(result, &headShort))
        return result;
    result.bases = baseRefs(result.root);
    result.baseRef = baseRef.trimmed().isEmpty() ? result.bases.value(0) : baseRef.trimmed();
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

void GitRepo::load(const QString &startPath, DiffMode mode, const QString &baseRef)
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
    watcher->setFuture(QtConcurrent::run([startPath, mode, baseRef]() {
        return loadGit(startPath, mode, baseRef);
    }));
}
