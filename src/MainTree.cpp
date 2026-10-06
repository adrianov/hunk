#include "MainWindow.hpp"

#include <QHash>
#include <QLineEdit>
#include <QTreeWidget>

#include <algorithm>

namespace {

struct Group {
    QString folder;
    QList<int> files;
};

bool folderBefore(const Group &left, const Group &right)
{
    if (left.folder.isEmpty())
        return !right.folder.isEmpty();
    if (right.folder.isEmpty())
        return false;
    return left.folder.localeAwareCompare(right.folder) < 0;
}

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

QList<Group> collectGroups(const DiffDoc &doc, const QString &query)
{
    QList<Group> groups;
    QHash<QString, int> index;
    for (int fileIndex = 0; fileIndex < doc.files.size(); ++fileIndex) {
        const FileDiff &file = doc.files.at(fileIndex);
        if (matchesQuery(file, query))
            addFile(&groups, &index, fileIndex, file.folder());
    }
    std::sort(groups.begin(), groups.end(), folderBefore);
    return groups;
}

QString fileLabel(const FileDiff &file)
{
    if (!file.adds && !file.dels)
        return file.shortName();
    return file.shortName() + QStringLiteral("    +%1  −%2").arg(file.adds).arg(file.dels);
}

void addFileItem(QTreeWidgetItem *folderItem, const FileDiff &file, int fileIndex)
{
    auto *item = new QTreeWidgetItem(folderItem, {fileLabel(file)});
    item->setData(0, Qt::UserRole, fileIndex);
    item->setToolTip(0, file.title());
    if (file.added)
        item->setForeground(0, QColor(QStringLiteral("#3fb950")));
    else if (file.removed)
        item->setForeground(0, QColor(QStringLiteral("#f85149")));
}

void addFolder(QTreeWidget *tree, const Group &group, const DiffDoc &doc)
{
    const QString name = group.folder.isEmpty() ? QStringLiteral("(root)") : group.folder;
    auto *folderItem = new QTreeWidgetItem(tree, {name});
    folderItem->setData(0, Qt::UserRole, -1);
    folderItem->setForeground(0, QColor(QStringLiteral("#858585")));
    for (int fileIndex : group.files)
        addFileItem(folderItem, doc.files.at(fileIndex), fileIndex);
}

} // namespace

void MainWindow::rebuildTree()
{
    m_navLock = true;
    m_tree->clear();
    for (const Group &group : collectGroups(m_doc, m_filter->text().trimmed()))
        addFolder(m_tree, group, m_doc);
    m_tree->expandAll();
    m_navLock = false;
}

void MainWindow::selectTreeFile(int file)
{
    if (m_navLock)
        return;
    m_navLock = true;
    for (QTreeWidgetItem *item : m_tree->findItems(QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive)) {
        if (item->data(0, Qt::UserRole).toInt() == file) {
            m_tree->setCurrentItem(item);
            m_tree->scrollToItem(item);
            break;
        }
    }
    m_navLock = false;
}
