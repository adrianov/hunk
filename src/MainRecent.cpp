// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include <QAction>
#include <QDir>
#include <QMenu>
#include <QSettings>

namespace {

constexpr int kRecentMax = 10;

QString recentLabel(const QString &path)
{
    const QString home = QDir::homePath();
    if (path == home)
        return QStringLiteral("~");
    if (path.startsWith(home + QLatin1Char('/')))
        return QStringLiteral("~") + path.mid(home.size());
    return path;
}

QStringList storedRecent()
{
    QSettings settings;
    QStringList paths = settings.value(QStringLiteral("recentRepos")).toStringList();
    if (!paths.isEmpty())
        return paths;
    const QString last = settings.value(QStringLiteral("lastRepo")).toString();
    if (!last.isEmpty())
        paths.append(QDir::cleanPath(last));
    return paths;
}

QStringList keptRecent(const QStringList &paths)
{
    QStringList kept;
    for (const QString &path : paths) {
        if (QDir(path).exists())
            kept.append(path);
    }
    return kept;
}

void saveRecent(bool smoke, const QStringList &kept)
{
    if (smoke)
        return;
    QSettings settings;
    if (kept != settings.value(QStringLiteral("recentRepos")).toStringList())
        settings.setValue(QStringLiteral("recentRepos"), kept);
}

} // namespace

void MainWindow::rememberRecent(const QString &root)
{
    if (m_smoke || root.isEmpty())
        return;
    QSettings settings;
    QStringList paths = settings.value(QStringLiteral("recentRepos")).toStringList();
    const QString clean = QDir::cleanPath(root);
    if (!paths.isEmpty() && paths.first() == clean)
        return;
    paths.removeAll(clean);
    paths.prepend(clean);
    while (paths.size() > kRecentMax)
        paths.removeLast();
    settings.setValue(QStringLiteral("recentRepos"), paths);
}

void MainWindow::openRecent()
{
    const auto *action = qobject_cast<const QAction *>(sender());
    if (!action)
        return;
    const QString path = action->data().toString();
    if (!path.isEmpty())
        openAt(path);
}

void MainWindow::addRecent(const QString &path)
{
    auto *action = m_recent->addAction(recentLabel(path));
    action->setData(path);
    connect(action, &QAction::triggered, this, &MainWindow::openRecent);
}

void MainWindow::fillRecent()
{
    m_recent->clear();
    const QStringList kept = keptRecent(storedRecent());
    saveRecent(m_smoke, kept);
    if (kept.isEmpty()) {
        m_recent->addAction(QStringLiteral("No recent projects"))->setEnabled(false);
        return;
    }
    for (const QString &path : kept)
        addRecent(path);
}
