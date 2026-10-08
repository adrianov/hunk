#include "MainWindow.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace {

constexpr int kWatchCap = 8192;

struct Walk {
    QSet<QString> have;
    QStringList pending;
    QStringList extra;
    bool capped = false;
};

void queueDir(Walk *walk, const QSet<QString> &ignored, const QString &path)
{
    if (walk->have.contains(path) || ignored.contains(path))
        return;
    if (walk->have.size() >= kWatchCap) {
        walk->capped = true;
        return;
    }
    walk->have.insert(path);
    walk->extra.append(path);
    walk->pending.append(path);
}

void scanDir(Walk *walk, const QSet<QString> &ignored, const QString &dir, const QString &skip)
{
    for (const QFileInfo &info : QDir(dir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
        if (walk->capped)
            return;
        if (!skip.isEmpty() && info.fileName() == skip)
            continue;
        queueDir(walk, ignored, QDir::cleanPath(info.absoluteFilePath()));
    }
}

bool addUnwatched(QFileSystemWatcher *disk, const QString &dir, const QString &skip, const QSet<QString> &ignored)
{
    Walk walk;
    walk.have = QSet<QString>(disk->directories().cbegin(), disk->directories().cend());
    walk.pending << dir;
    while (!walk.pending.isEmpty() && !walk.capped)
        scanDir(&walk, ignored, walk.pending.takeLast(), skip);
    if (walk.extra.isEmpty())
        return !walk.capped;
    return disk->addPaths(walk.extra).isEmpty() && !walk.capped;
}

} // namespace

bool MainWindow::growDisk(const QString &dir, const QString &skip)
{
    return addUnwatched(&m_disk, dir, skip, m_ignored);
}
