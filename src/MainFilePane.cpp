// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffColors.hpp"
#include "MainSeen.hpp"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

QLineEdit *fileField(QWidget *parent, const QString &placeholder)
{
    auto *field = new QLineEdit(parent);
    field->setPlaceholderText(placeholder);
    field->setClearButtonEnabled(true);
    return field;
}

QTreeWidget *fileTree(QWidget *parent)
{
    auto *tree = new QTreeWidget(parent);
    tree->setHeaderHidden(true);
    tree->setIndentation(14);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    installSeenRows(tree);
    return tree;
}

QLabel *lineStat(QWidget *parent)
{
    auto *stat = new QLabel(parent);
    stat->setTextFormat(Qt::RichText);
    stat->setContentsMargins(8, 0, 8, 0);
    return stat;
}

QPushButton *hideReset(QWidget *parent)
{
    auto *button = new QPushButton(parent);
    button->setCursor(Qt::PointingHandCursor);
    button->setVisible(false);
    return button;
}

void stackFields(QVBoxLayout *layout, QWidget *parent, QLineEdit **filter, QLineEdit **search)
{
    *filter = fileField(parent, QStringLiteral("Filter files"));
    *search = fileField(parent, QStringLiteral("Search in files"));
    layout->addWidget(*filter);
    layout->addWidget(*search);
}

QWidget *statBar(QWidget *parent, QLabel *stat, QPushButton *unhide)
{
    auto *bar = new QWidget(parent);
    auto *stats = new QHBoxLayout(bar);
    stats->setContentsMargins(0, 4, 4, 4);
    stats->setSpacing(0);
    stats->addWidget(stat, 0, Qt::AlignVCenter);
    stats->addStretch(1);
    stats->addWidget(unhide, 0, Qt::AlignVCenter);
    return bar;
}

} // namespace

QWidget *MainWindow::makeFilePane()
{
    m_tree = fileTree(this);
    auto *left = new QWidget(this);
    auto *layout = new QVBoxLayout(left);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    stackFields(layout, this, &m_filter, &m_search);
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
