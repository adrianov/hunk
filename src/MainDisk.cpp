#include "MainWindow.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace {

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

bool watchDeeper(const QString &git, const QString &path)
{
    if (!QFileInfo(path).isDir() || path == git)
        return false;
    const bool inside = !git.isEmpty() && path.startsWith(git + QLatin1Char('/'));
    if (!inside)
        return true;
    const QString refs = git + QStringLiteral("/refs");
    return path == refs || path.startsWith(refs + QLatin1Char('/'));
}

void queueDir(QSet<QString> *have, QStringList *pending, QStringList *extra, const QString &path)
{
    if (have->contains(path))
        return;
    have->insert(path);
    extra->append(path);
    pending->append(path);
}

void scanDir(QSet<QString> *have, QStringList *pending, QStringList *extra, const QString &dir, const QString &skip)
{
    for (const QFileInfo &info : QDir(dir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
        if (!skip.isEmpty() && info.fileName() == skip)
            continue;
        queueDir(have, pending, extra, info.absoluteFilePath());
    }
}

void addUnwatched(QFileSystemWatcher *disk, const QString &dir, const QString &skip)
{
    QSet<QString> have(disk->directories().cbegin(), disk->directories().cend());
    QStringList pending{dir};
    QStringList extra;
    while (!pending.isEmpty())
        scanDir(&have, &pending, &extra, pending.takeLast(), skip);
    if (!extra.isEmpty())
        disk->addPaths(extra);
}

} // namespace

void MainWindow::clearDisk()
{
    const QStringList watched = m_disk.directories() + m_disk.files();
    if (!watched.isEmpty())
        m_disk.removePaths(watched);
    m_gitDir.clear();
}

void MainWindow::armDisk()
{
    if (m_smoke || m_root.isEmpty())
        return;
    clearDisk();
    m_gitDir = gitDirOf(m_root);
    m_disk.addPath(m_root);
    addUnwatched(&m_disk, m_root, QStringLiteral(".git"));
    if (m_gitDir.isEmpty())
        return;
    m_disk.addPath(m_gitDir);
    const QString refs = m_gitDir + QStringLiteral("/refs");
    if (!QDir(refs).exists())
        return;
    m_disk.addPath(refs);
    addUnwatched(&m_disk, refs, {});
}

void MainWindow::noteDisk(const QString &path)
{
    if (watchDeeper(m_gitDir, path)) {
        const QString refs = m_gitDir + QStringLiteral("/refs");
        const bool refsTree = path == refs || path.startsWith(refs + QLatin1Char('/'));
        addUnwatched(&m_disk, path, refsTree ? QString() : QStringLiteral(".git"));
    }
    scheduleWatch();
}
