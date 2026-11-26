// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"
#include "MainWindow.hpp"

#include <QFutureWatcher>
#include <QtConcurrent>

QStringList grepFiles(const QString &root, const QString &query)
{
    if (root.isEmpty() || query.isEmpty())
        return {};
    const GitCmd found = runGit(root, {QStringLiteral("grep"), QStringLiteral("-I"), QStringLiteral("-i"),
                                        QStringLiteral("-F"), QStringLiteral("-l"), QStringLiteral("--untracked"),
                                        QStringLiteral("-e"), query});
    if (found.code != 0 && found.code != 1)
        return {};
    return found.out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
}

namespace {

QStringList nullPaths(const QString &out)
{
    QStringList paths = out.split(QChar(u'\0'), Qt::SkipEmptyParts);
    paths.removeIf([](const QString &path) { return path.endsWith(QLatin1Char('/')); });
    return paths;
}

bool nestedUnder(const QString &path, const QString &folder)
{
    if (folder.isEmpty())
        return path.contains(QLatin1Char('/'));
    const QString prefix = folder + QLatin1Char('/');
    return !path.startsWith(prefix) || path.mid(prefix.size()).contains(QLatin1Char('/'));
}

} // namespace

QStringList dirFiles(const QString &root, const QString &folder)
{
    if (root.isEmpty())
        return {};
    const QString glob = folder.isEmpty() ? QStringLiteral("*") : folder + QStringLiteral("/*");
    const GitCmd listed = runGit(root, {QStringLiteral("ls-files"), QStringLiteral("-z"), QStringLiteral("--cached"),
                                        QStringLiteral("--others"), QStringLiteral("--exclude-standard"),
                                        QStringLiteral("--"), glob});
    if (listed.code != 0)
        return {};
    QStringList paths = nullPaths(listed.out);
    paths.removeIf([&](const QString &path) { return nestedUnder(path, folder); });
    return paths;
}

QList<FileDiff> dirPlain(const QString &root, const QString &folder)
{
    QList<FileDiff> files;
    for (const QString &path : dirFiles(root, folder)) {
        FileDiff plain;
        if (loadPlain(root, path, &plain))
            files.append(plain);
    }
    return files;
}

void MainWindow::armHits(const QString &search)
{
    ++m_grepGen;
    if (m_hitsQuery != search)
        m_hits.clear();
    m_hitsQuery = search;
    m_hitsRoot = m_root;
    if (!m_root.isEmpty())
        m_grep.start();
}

void MainWindow::armGrepTimer()
{
    m_grep.setSingleShot(true);
    m_grep.setInterval(250);
    connect(&m_grep, &QTimer::timeout, this, &MainWindow::launchHits);
}

void MainWindow::applyHits(int token, const QStringList &hits)
{
    if (token != m_grepGen)
        return;
    m_hits = hits;
    fillTree(true);
}

void MainWindow::launchHits()
{
    const QString search = m_hitsQuery;
    const QString root = m_root;
    const int token = m_grepGen;
    if (search.isEmpty() || root.isEmpty())
        return;
    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher, token]() {
        applyHits(token, watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([root, search]() { return grepFiles(root, search); }));
}
