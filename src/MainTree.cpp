// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "DiffColors.hpp"
#include "MainSeen.hpp"
#include "MdView.hpp"

#include <QHash>
#include <QLineEdit>
#include <QStringList>
#include <QTreeWidget>

namespace {

struct Group {
    QString folder;
    QList<int> files;
};

bool matchesQuery(const FileDiff &file, const QString &query)
{
    if (query.isEmpty())
        return true;
    return file.title().contains(query, Qt::CaseInsensitive) || file.shortName().contains(query, Qt::CaseInsensitive);
}

void addFile(QList<Group> *groups, QHash<QString, int> *index, int fileIndex, const QString &folder)
{
    if (!index->contains(folder)) {
        index->insert(folder, groups->size());
        groups->push_back(Group{folder, {}});
    }
    (*groups)[index->value(folder)].files.push_back(fileIndex);
}

QString diffText(const FileDiff &file)
{
    QStringList lines;
    for (const DiffRow &row : file.rows) {
        if (row.leftNum > 0)
            lines.append(row.leftText);
        if (row.rightNum > 0 && row.rightText != row.leftText)
            lines.append(row.rightText);
    }
    return lines.join(QLatin1Char('\n'));
}

bool matchesBody(const FileDiff &file, const QString &query, QHash<QString, QString> *cache)
{
    if (query.isEmpty())
        return true;
    if (file.binary)
        return false;
    auto found = cache->find(file.path());
    if (found == cache->end())
        found = cache->insert(file.path(), diffText(file));
    return found->contains(query, Qt::CaseInsensitive);
}

QList<Group> collectGroups(const DiffDoc &doc, const QString &query, const QString &search,
                           const QHash<QString, QString> &hidden, QHash<QString, QString> *cache)
{
    QList<Group> groups;
    QHash<QString, int> index;
    for (int fileIndex : listedOrder(doc)) {
        const FileDiff &file = doc.files.at(fileIndex);
        if (fileHidden(file, hidden) || !matchesQuery(file, query) || !matchesBody(file, search, cache))
            continue;
        addFile(&groups, &index, fileIndex, file.folder());
    }
    return groups;
}

void tagCounts(QTreeWidgetItem *item, const FileDiff &file)
{
    if (!file.adds && !file.dels)
        return;
    item->setData(0, Qt::UserRole + 1, file.adds);
    item->setData(0, Qt::UserRole + 2, file.dels);
}

void addFileItem(QTreeWidgetItem *folderItem, const FileDiff &file, int fileIndex, const QHash<QString, QString> &seen,
                 const QWidget *tree)
{
    auto *item = new QTreeWidgetItem(folderItem, {file.shortName()});
    item->setData(0, Qt::UserRole, fileIndex);
    tagCounts(item, file);
    item->setToolTip(0, file.title());
    if (file.added)
        item->setForeground(0, kAddFg);
    else if (file.removed)
        item->setForeground(0, kDelFg);
    if (!staleFile(file, seen))
        return;
    item->setBackground(0, staleBg(tree));
    item->setToolTip(0, file.title() + QStringLiteral("\nChanged since you reviewed it"));
}

void addFolder(QTreeWidget *tree, const Group &group, const DiffDoc &doc, const QHash<QString, QString> &seen)
{
    const QString name = group.folder.isEmpty() ? QStringLiteral("(root)") : group.folder;
    auto *folderItem = new QTreeWidgetItem(tree, {name});
    folderItem->setData(0, Qt::UserRole, -1);
    folderItem->setForeground(0, kMuted);
    for (int fileIndex : group.files)
        addFileItem(folderItem, doc.files.at(fileIndex), fileIndex, seen, tree);
}

} // namespace

void MainWindow::loadSeen()
{
    m_seen = readSeen(m_root);
    m_hidden = readHidden(m_root);
}

void MainWindow::markSeenFile(int file)
{
    if (m_smoke || m_root.isEmpty() || file < 0 || file >= m_doc.files.size())
        return;
    const FileDiff &diff = m_doc.files.at(file);
    const QString stamp = changeStamp(diff);
    if (m_seen.value(diff.path()) == stamp)
        return;
    m_seen.insert(diff.path(), stamp);
    writeSeen(m_root, m_seen);
    clearSeenMark(m_tree, file);
}

void MainWindow::rebuildTree()
{
    m_diff->setSearch(m_search->text().trimmed());
    m_navLock = true;
    m_tree->setProperty("checkHot", 0);
    m_tree->clear();
    for (const Group &group :
         collectGroups(m_doc, m_filter->text().trimmed(), m_search->text().trimmed(), m_hidden, &m_bodies))
        addFolder(m_tree, group, m_doc, m_seen);
    m_tree->expandAll();
    m_navLock = false;
}

void MainWindow::selectTreeFile(int file)
{
    if (m_navLock)
        return;
    markSeenFile(file);
    m_navLock = true;
    for (QTreeWidgetItem *item : m_tree->findItems(QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive)) {
        if (item->data(0, Qt::UserRole).toInt() == file) {
            m_tree->setCurrentItem(item);
            m_tree->scrollToItem(item);
            break;
        }
    }
    m_navLock = false;
    m_md->track(file, m_doc, m_root);
}
