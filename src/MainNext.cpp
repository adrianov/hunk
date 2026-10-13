// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"

#include <QTreeWidget>

namespace {

QList<int> treeFiles(QTreeWidget *tree)
{
    QList<int> files;
    for (QTreeWidgetItem *item : tree->findItems(QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive)) {
        const int index = item->data(0, Qt::UserRole).toInt();
        if (index >= 0)
            files.append(index);
    }
    return files;
}

int neighborFile(const QList<int> &files, int file)
{
    const int at = files.indexOf(file);
    if (at < 0)
        return -1;
    if (at + 1 < files.size())
        return files.at(at + 1);
    return at > 0 ? files.at(at - 1) : -1;
}

} // namespace

int MainWindow::nextListed(int file) const
{
    return neighborFile(treeFiles(m_tree), file);
}

void MainWindow::showListed(int file)
{
    if (file < 0)
        return;
    selectTreeFile(file);
    m_navLock = true;
    m_diff->showFile(file);
    m_navLock = false;
}
