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
