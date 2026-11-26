// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "GitDetail.hpp"
#include "MainSeen.hpp"
#include "MdView.hpp"

#include <QFutureWatcher>
#include <QtConcurrent>

namespace {

QString pathFolder(const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    return slash < 0 ? QString() : path.left(slash);
}

QString stayPath(const DiffDoc &doc, const QString &folder)
{
    for (const FileDiff &file : doc.files) {
        if (!file.opened && file.folder() == folder)
            return file.path();
    }
    return {};
}

QStringList keepOutside(const QStringList &opened, const QString &folder)
{
    QStringList keep;
    for (const QString &path : opened) {
        if (pathFolder(path) != folder)
            keep.append(path);
    }
    return keep;
}

void dropOpenedIn(DiffDoc *doc, const QString &folder)
{
    for (int index = doc->files.size() - 1; index >= 0; --index) {
        const FileDiff &file = doc->files.at(index);
        if (file.opened && file.folder() == folder)
            doc->files.removeAt(index);
    }
}

bool dirStillOpen(const DiffDoc &doc, const QString &folder)
{
    for (const FileDiff &diff : doc.files) {
        if (diff.opened && diff.folder() == folder)
            return true;
    }
    return false;
}

} // namespace

void MainWindow::dropOpened()
{
    dropDirCache();
    m_opened.clear();
    m_fullDirs.clear();
}

void MainWindow::dropDirCache()
{
    ++m_dirGen;
    m_dirPaths.clear();
}

void MainWindow::fillDirFiles()
{
    dropDirCache();
    for (const QString &folder : m_fullDirs)
        launchDir(folder, false);
}

void MainWindow::forgetDir(const QString &folder)
{
    if (!dirStillOpen(m_doc, folder))
        m_fullDirs.remove(folder);
}

void MainWindow::applyDirPaths(const QStringList &paths, bool focus)
{
    const int before = m_doc.files.size();
    int first = -1;
    for (const QString &path : paths) {
        const int file = plainIndex(path);
        if (file < 0)
            continue;
        if (first < 0)
            first = file;
    }
    if (m_doc.files.size() != before)
        showPlain();
    if (!focus || first < 0)
        return;
    m_diff->showFile(first);
    selectTreeFile(first);
    m_md->track(first, m_doc, m_root);
}

void MainWindow::showDirFiles(const QString &folder)
{
    m_fullDirs.insert(folder);
    const auto found = m_dirPaths.constFind(folder);
    if (found != m_dirPaths.cend())
        applyDirPaths(found.value(), true);
    else
        launchDir(folder, true);
}

void MainWindow::hideDirFiles(const QString &folder)
{
    m_fullDirs.remove(folder);
    const QString stay = stayPath(m_doc, folder);
    m_opened = keepOutside(m_opened, folder);
    dropOpenedIn(&m_doc, folder);
    finishClose(fileNamed(m_doc, stay));
}

void MainWindow::toggleDirFiles(const QString &folder)
{
    if (m_fullDirs.contains(folder))
        hideDirFiles(folder);
    else
        showDirFiles(folder);
}

void MainWindow::applyDir(int token, const QString &folder, const QStringList &paths, bool focus)
{
    if (token != m_dirGen || !m_fullDirs.contains(folder))
        return;
    m_dirPaths.insert(folder, paths);
    applyDirPaths(paths, focus);
}

void MainWindow::launchDir(const QString &folder, bool focus)
{
    const QString root = m_root;
    const int token = m_dirGen;
    if (root.isEmpty())
        return;
    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher, token, folder, focus]() {
        applyDir(token, folder, watcher->result(), focus);
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([root, folder]() { return dirFiles(root, folder); }));
}
