// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffColors.hpp"
#include "MainSeen.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

QLineEdit *fileFilter(QWidget *parent)
{
    auto *filter = new QLineEdit(parent);
    filter->setPlaceholderText(QStringLiteral("Filter files"));
    filter->setClearButtonEnabled(true);
    return filter;
}

QTreeWidget *fileTree(QWidget *parent)
{
    auto *tree = new QTreeWidget(parent);
    tree->setHeaderHidden(true);
    tree->setIndentation(14);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    installSeenRows(tree);
    return tree;
}

QLabel *lineStat(QWidget *parent)
{
    auto *stat = new QLabel(parent);
    stat->setTextFormat(Qt::RichText);
    stat->setContentsMargins(8, 4, 8, 4);
    return stat;
}

QPushButton *hideReset(QWidget *parent)
{
    auto *button = new QPushButton(parent);
    button->setFlat(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setVisible(false);
    return button;
}

QWidget *statBar(QWidget *parent, QLabel *stat, QPushButton *unhide)
{
    auto *bar = new QWidget(parent);
    auto *stats = new QHBoxLayout(bar);
    stats->setContentsMargins(0, 0, 4, 0);
    stats->setSpacing(0);
    stats->addWidget(stat, 0, Qt::AlignVCenter);
    stats->addStretch(1);
    stats->addWidget(unhide, 0, Qt::AlignVCenter);
    return bar;
}

QString listedPath(const QTreeWidget *tree, const DiffDoc &doc, const QPoint &pos)
{
    QTreeWidgetItem *item = tree->itemAt(pos);
    if (!item)
        return {};
    const int file = item->data(0, Qt::UserRole).toInt();
    if (file < 0 || file >= doc.files.size())
        return {};
    return doc.files.at(file).path();
}

} // namespace

QWidget *MainWindow::makeFilePane()
{
    m_filter = fileFilter(this);
    m_tree = fileTree(this);
    auto *left = new QWidget(this);
    auto *layout = new QVBoxLayout(left);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_filter);
    layout->addWidget(m_tree, 1);
    m_stat = lineStat(left);
    m_unhide = hideReset(left);
    layout->addWidget(statBar(left, m_stat, m_unhide));
    showLineStat();
    return left;
}

int MainWindow::showLineStat()
{
    int adds = 0;
    int dels = 0;
    int hidden = 0;
    int visible = 0;
    for (const FileDiff &file : m_doc.files) {
        if (fileHidden(file, m_hidden)) {
            ++hidden;
            continue;
        }
        ++visible;
        adds += file.adds;
        dels += file.dels;
    }
    if (m_stat) {
        m_stat->setText(QStringLiteral("<span style=\"color:%1\">+%2</span>  <span style=\"color:%3\">−%4</span>")
                            .arg(kAddFg.name(), QString::number(adds), kDelFg.name(), QString::number(dels)));
    }
    if (m_unhide) {
        m_unhide->setText(hidden == 1 ? QStringLiteral("Unhide 1 reviewed file")
                                      : QStringLiteral("Unhide %1 reviewed files").arg(hidden));
        m_unhide->setVisible(hidden > 0);
    }
    return visible;
}

void MainWindow::filePathMenu(const QPoint &pos)
{
    const QString path = listedPath(m_tree, m_doc, pos);
    if (path.isEmpty())
        return;
    QMenu menu(m_tree);
    menu.addAction(QStringLiteral("Copy Relative Path"));
    if (menu.exec(m_tree->viewport()->mapToGlobal(pos)) != menu.actions().constFirst())
        return;
    QGuiApplication::clipboard()->setText(path);
}
