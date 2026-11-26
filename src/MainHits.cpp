// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffColors.hpp"
#include "MainSeen.hpp"

#include <QHash>
#include <QLineEdit>
#include <QSet>
#include <QTreeWidget>

#include <algorithm>

bool bodyMatches(const FileDiff &file, const QString &query, QHash<QString, QString> *cache);

namespace {

struct HitGroup {
    QString folder;
    QStringList paths;
};

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

bool rootFirst(const HitGroup &left, const HitGroup &right)
{
    if (left.folder.isEmpty())
        return !right.folder.isEmpty();
    if (right.folder.isEmpty())
        return false;
    return left.folder.localeAwareCompare(right.folder) < 0;
}

void addGrouped(QList<HitGroup> *groups, QHash<QString, int> *index, const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    const QString folder = slash < 0 ? QString() : path.left(slash);
    if (!index->contains(folder)) {
        index->insert(folder, groups->size());
        groups->push_back(HitGroup{folder, {}});
    }
    (*groups)[index->value(folder)].paths.append(path);
}

void collectHits(QList<HitGroup> *groups, const QStringList &hits, const QString &filter)
{
    QHash<QString, int> index;
    for (const QString &path : hits) {
        if (filter.isEmpty() || path.contains(filter, Qt::CaseInsensitive))
            addGrouped(groups, &index, path);
    }
    std::sort(groups->begin(), groups->end(), rootFirst);
}

void addHitFile(QTreeWidgetItem *folder, const QString &path, const DiffDoc &doc)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    auto *item = new QTreeWidgetItem(folder, {slash < 0 ? path : path.mid(slash + 1)});
    const int index = fileNamed(doc, path);
    item->setData(0, Qt::UserRole, index >= 0 ? index : -2);
    item->setData(0, Qt::UserRole + 3, path);
    item->setToolTip(0, path);
}

void addHitFolder(QTreeWidget *tree, const HitGroup &group, const DiffDoc &doc)
{
    const QString name = group.folder.isEmpty() ? QStringLiteral("(root)") : group.folder;
    auto *folder = new QTreeWidgetItem(tree, {name});
    folder->setData(0, Qt::UserRole, -1);
    folder->setForeground(0, kMuted);
    folder->setToolTip(0, group.folder);
    for (const QString &path : group.paths)
        addHitFile(folder, path, doc);
}

bool changesetHit(const DiffDoc &doc, const QString &query, const QHash<QString, QString> &hidden,
                  const QSet<QString> &changeset, QHash<QString, QString> *cache)
{
    for (const FileDiff &file : doc.files) {
        if (!changeset.contains(file.path()) || fileHidden(file, hidden))
            continue;
        if (bodyMatches(file, query, cache))
            return true;
    }
    return false;
}

QStringList listedHits(const QStringList &hits, const DiffDoc &doc, const QString &search, QHash<QString, QString> *cache)
{
    QSet<QString> seen(hits.cbegin(), hits.cend());
    QStringList paths = hits;
    for (const FileDiff &file : doc.files) {
        const QString path = file.path();
        if (seen.contains(path) || !bodyMatches(file, search, cache))
            continue;
        seen.insert(path);
        paths.append(path);
    }
    return paths;
}

} // namespace

bool bodyMatches(const FileDiff &file, const QString &query, QHash<QString, QString> *cache)
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

void MainWindow::addHits()
{
    QList<HitGroup> groups;
    collectHits(&groups, listedHits(m_hits, m_doc, m_search->text().trimmed(), &m_bodies), m_filter->text().trimmed());
    for (const HitGroup &group : groups)
        addHitFolder(m_tree, group, m_doc);
}

void MainWindow::dropHits()
{
    ++m_grepGen;
    m_grep.stop();
    m_hitsQuery.clear();
    m_hitsRoot.clear();
    m_hits.clear();
}

void MainWindow::dropLoad()
{
    m_changeset.clear();
    dropHits();
}

void MainWindow::takeChangeset()
{
    m_changeset.clear();
    for (const FileDiff &file : m_doc.files)
        m_changeset.insert(file.path());
    if (!m_hitsQuery.isEmpty())
        m_hitsRoot.clear();
}

bool MainWindow::projectSearch()
{
    const QString search = m_search->text().trimmed();
    if (search.isEmpty() || changesetHit(m_doc, search, m_hidden, m_changeset, &m_bodies)) {
        dropHits();
        return false;
    }
    if (m_hitsQuery != search || m_hitsRoot != m_root)
        armHits(search);
    return true;
}

QString MainWindow::hitPath(const QTreeWidgetItem *item) const
{
    if (!item)
        return {};
    const QString path = item->data(0, Qt::UserRole + 3).toString();
    if (!path.isEmpty())
        return path;
    for (int child = 0; child < item->childCount(); ++child) {
        const QString found = hitPath(item->child(child));
        if (!found.isEmpty())
            return found;
    }
    return {};
}
