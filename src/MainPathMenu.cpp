// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QSet>
#include <QTreeWidget>

namespace {

QString itemPath(const QTreeWidgetItem *item, const DiffDoc &doc)
{
    const int file = item->data(0, Qt::UserRole).toInt();
    if (file >= 0 && file < doc.files.size())
        return doc.files.at(file).path();
    const QString hit = item->data(0, Qt::UserRole + 3).toString();
    if (!hit.isEmpty())
        return hit;
    if (item->parent())
        return {};
    const QString folder = item->text(0);
    return folder == QLatin1String("(root)") ? QStringLiteral(".") : folder;
}

void collectPicked(QStringList *paths, const QTreeWidgetItem *item, const DiffDoc &doc)
{
    if (item->isSelected()) {
        const QString path = itemPath(item, doc);
        if (!path.isEmpty())
            paths->append(path);
    }
    for (int index = 0; index < item->childCount(); ++index)
        collectPicked(paths, item->child(index), doc);
}

QString listedPath(const QTreeWidget *tree, const DiffDoc &doc, const QPoint &pos)
{
    QTreeWidgetItem *hit = tree->itemAt(pos);
    if (!hit)
        return {};
    if (!hit->isSelected() || tree->selectedItems().size() < 2)
        return itemPath(hit, doc);
    QStringList paths;
    for (int index = 0; index < tree->topLevelItemCount(); ++index)
        collectPicked(&paths, tree->topLevelItem(index), doc);
    return paths.join(QLatin1Char('\n'));
}

bool menuFolder(const QTreeWidgetItem *hit, QString *folder)
{
    if (!hit || hit->data(0, Qt::UserRole).toInt() != -1)
        return false;
    if (!hit->data(0, Qt::UserRole + 3).toString().isEmpty())
        return false;
    const QString name = hit->text(0);
    *folder = name == QLatin1String("(root)") ? QString() : name;
    return true;
}

QAction *dirToggle(QMenu *menu, QTreeWidgetItem *hit, const QSet<QString> &fullDirs, QString *folder)
{
    if (!menuFolder(hit, folder))
        return nullptr;
    return menu->addAction(fullDirs.contains(*folder) ? QStringLiteral("Hide Unchanged Files")
                                                      : QStringLiteral("Show All Files in the Directory"));
}

} // namespace

void MainWindow::filePathMenu(const QPoint &pos)
{
    QTreeWidgetItem *hit = m_tree->itemAt(pos);
    const QString path = listedPath(m_tree, m_doc, pos);
    if (path.isEmpty())
        return;
    QMenu menu(m_tree);
    QAction *copy = menu.addAction(QStringLiteral("Copy Relative Path"));
    QString folder;
    QAction *toggle = dirToggle(&menu, hit, m_fullDirs, &folder);
    QAction *picked = menu.exec(m_tree->viewport()->mapToGlobal(pos));
    if (picked == copy)
        QGuiApplication::clipboard()->setText(path);
    else if (picked && picked == toggle)
        toggleDirFiles(folder);
}
