// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainRef.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMenu>

bool refFieldMenu(QLineEdit *edit, const QPoint &global)
{
    QMenu *menu = edit->createStandardContextMenu();
    auto *copy = new QAction(QStringLiteral("Copy Branch Name"), menu);
    QAction *first = menu->actions().value(0);
    menu->insertAction(first, copy);
    if (first)
        menu->insertSeparator(first);
    if (menu->exec(global) == copy)
        QGuiApplication::clipboard()->setText(edit->text());
    delete menu;
    return true;
}

namespace {

QLineEdit *refEdit()
{
    auto *edit = qobject_cast<QLineEdit *>(QApplication::focusWidget());
    if (edit && qobject_cast<QComboBox *>(edit->parentWidget()))
        return edit;
    return nullptr;
}

bool takeCopy(QMenu *menu, const QPoint &global)
{
    QLineEdit *edit = refEdit();
    if (edit)
        edit->blockSignals(true);
    const bool take = menu->exec(global) == menu->actions().constFirst();
    if (edit)
        edit->blockSignals(false);
    return take;
}

void listMenu(QAbstractItemView *popup, const QPoint &pos)
{
    const QModelIndex index = popup->indexAt(pos);
    const QString name = index.data(Qt::DisplayRole).toString();
    if (!index.isValid() || name.isEmpty())
        return;
    QMenu menu;
    menu.addAction(QStringLiteral("Copy Branch Name"));
    if (takeCopy(&menu, popup->viewport()->mapToGlobal(pos)))
        QGuiApplication::clipboard()->setText(name);
}

} // namespace

void armRefList(QAbstractItemView *popup)
{
    popup->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(popup->viewport(), &QWidget::customContextMenuRequested, popup, [popup](const QPoint &pos) {
        listMenu(popup, pos);
    });
}
