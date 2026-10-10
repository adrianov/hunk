// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "MainSeen.hpp"
#include "MdView.hpp"

#include <QMouseEvent>
#include <QTreeWidget>

QString MainWindow::fileTip(const QTreeWidgetItem *item) const
{
    const int file = item->data(0, Qt::UserRole).toInt();
    if (file < 0 || file >= m_doc.files.size())
        return item->toolTip(0);
    const FileDiff &diff = m_doc.files.at(file);
    if (!staleFile(diff, m_seen))
        return diff.title();
    return diff.title() + QStringLiteral("\nChanged since you reviewed it");
}

namespace {

QTreeWidgetItem *checkItem(QTreeWidget *tree, const QPoint &pos)
{
    QTreeWidgetItem *item = tree->itemAt(pos);
    if (!item || item->data(0, Qt::UserRole).toInt() < 0)
        return nullptr;
    return fileCheckRect(tree->visualItemRect(item)).contains(pos) ? item : nullptr;
}

} // namespace

void MainWindow::hoverCheck(const QPoint &pos)
{
    QTreeWidgetItem *item = checkItem(m_tree, pos);
    const quintptr now = quintptr(item);
    const quintptr was = m_tree->property("checkHot").value<quintptr>();
    m_tree->viewport()->setCursor(item ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (was == now)
        return;
    if (auto *prior = reinterpret_cast<QTreeWidgetItem *>(was))
        prior->setToolTip(0, fileTip(prior));
    if (item)
        item->setToolTip(0, QStringLiteral("Mark as reviewed"));
    m_tree->setProperty("checkHot", now);
    m_tree->viewport()->update();
}

bool MainWindow::takeCheck(QEvent *event)
{
    const auto type = event->type();
    if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease && type != QEvent::MouseButtonDblClick)
        return false;
    auto *mouse = static_cast<QMouseEvent *>(event);
    QTreeWidgetItem *item = checkItem(m_tree, mouse->position().toPoint());
    if (!item || mouse->button() != Qt::LeftButton)
        return false;
    if (type == QEvent::MouseButtonPress)
        hideReviewed(item->data(0, Qt::UserRole).toInt());
    return true;
}

bool MainWindow::fileCheckEvent(QObject *object, QEvent *event)
{
    if (!m_tree || object != m_tree->viewport())
        return false;
    if (event->type() == QEvent::Leave) {
        hoverCheck(QPoint(-1, -1));
        return false;
    }
    if (event->type() == QEvent::MouseMove) {
        hoverCheck(static_cast<QMouseEvent *>(event)->position().toPoint());
        return false;
    }
    return takeCheck(event);
}

void MainWindow::applyHidden()
{
    if (!m_smoke && !m_root.isEmpty())
        writeHidden(m_root, m_hidden);
    rebuildTree();
    m_diff->setSkipped(hiddenIndexes(m_doc, m_hidden));
    keepShownFile();
    updateStatus();
}

void MainWindow::keepShownFile()
{
    const int shown = m_md->shownFile();
    if (shown < 0 || shown >= m_doc.files.size() || !fileHidden(m_doc.files.at(shown), m_hidden))
        return;
    m_md->leaveFile(shown);
}

void MainWindow::hideReviewed(int file)
{
    if (m_smoke || m_root.isEmpty() || file < 0 || file >= m_doc.files.size())
        return;
    const FileDiff &diff = m_doc.files.at(file);
    const QString stamp = changeStamp(diff);
    m_hidden.insert(diff.path(), stamp);
    if (m_seen.value(diff.path()) != stamp) {
        m_seen.insert(diff.path(), stamp);
        writeSeen(m_root, m_seen);
    }
    applyHidden();
}

void MainWindow::unhideReviewed()
{
    if (m_hidden.isEmpty())
        return;
    m_hidden.clear();
    applyHidden();
}

void MainWindow::revealReviewed(int file)
{
    if (file < 0 || file >= m_doc.files.size() || !fileHidden(m_doc.files.at(file), m_hidden))
        return;
    m_hidden.remove(m_doc.files.at(file).path());
    applyHidden();
}
