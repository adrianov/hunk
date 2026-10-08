#include "MainWindow.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

constexpr int kGapTries = 3;

QString gitSpec(const QString &root, const QString &line)
{
    const auto marker = QLatin1String("gitdir:");
    if (!line.startsWith(marker))
        return {};
    return QDir::cleanPath(QDir(root).absoluteFilePath(line.mid(marker.size()).trimmed()));
}

QString readGitDir(const QString &root, const QString &dot)
{
    QFile file(dot);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return gitSpec(root, QString::fromUtf8(file.readLine()).trimmed());
}

QString gitDirOf(const QString &root)
{
    const QString dot = root + QStringLiteral("/.git");
    if (QFileInfo(dot).isDir())
        return QFileInfo(dot).absoluteFilePath();
    return readGitDir(root, dot);
}

QString refsPathOf(const QString &git)
{
    return git.isEmpty() ? QString() : git + QStringLiteral("/refs");
}

bool isRefsPath(const QString &git, const QString &path)
{
    const QString refs = refsPathOf(git);
    return !refs.isEmpty() && (path == refs || path.startsWith(refs + QLatin1Char('/')));
}

bool isWatchedGitPath(const QString &git, const QString &path)
{
    if (path == git)
        return false;
    if (git.isEmpty() || !path.startsWith(git + QLatin1Char('/')))
        return true;
    return isRefsPath(git, path);
}

QString watchSkip(const QString &git, const QString &path)
{
    return isRefsPath(git, path) ? QString() : QStringLiteral(".git");
}

void dropWatched(QFileSystemWatcher *disk)
{
    const QStringList watched = disk->directories() + disk->files();
    if (!watched.isEmpty())
        disk->removePaths(watched);
}

} // namespace

void MainWindow::clearDisk()
{
    dropWatched(&m_disk);
    m_gitDir.clear();
    m_ignored.clear();
    m_diskGap = false;
    m_gapLeft = 0;
}

void MainWindow::markGap()
{
    if (m_diskGap)
        return;
    m_diskGap = true;
    m_gapLeft = kGapTries;
}

void MainWindow::armDisk(const QStringList &ignored)
{
    if (m_smoke || m_root.isEmpty())
        return;
    dropWatched(&m_disk);
    m_gitDir = gitDirOf(m_root);
    m_ignored = QSet<QString>(ignored.cbegin(), ignored.cend());
    if (watchTree() & watchRefs()) {
        m_diskGap = false;
        m_gapLeft = 0;
        return;
    }
    if (!m_diskGap)
        m_gapLeft = kGapTries;
    m_diskGap = true;
}

void MainWindow::noteDisk(const QString &path)
{
    if (QFileInfo(path).isDir() && isWatchedGitPath(m_gitDir, path)) {
        if (!growDisk(path, watchSkip(m_gitDir, path)))
            markGap();
    }
    scheduleWatch();
}

bool MainWindow::watchTree()
{
    return m_disk.addPath(m_root) & growDisk(m_root, watchSkip(m_gitDir, m_root));
}

bool MainWindow::watchRefs()
{
    if (m_gitDir.isEmpty())
        return true;
    bool covered = m_disk.addPath(m_gitDir);
    const QString refs = refsPathOf(m_gitDir);
    if (!QDir(refs).exists())
        return covered;
    covered = m_disk.addPath(refs) && covered;
    return growDisk(refs, watchSkip(m_gitDir, refs)) && covered;
}
