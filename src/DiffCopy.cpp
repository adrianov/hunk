// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>

namespace {

bool keepsCopy(QWidget *focus)
{
    for (QWidget *widget = focus; widget; widget = widget->parentWidget()) {
        if (qobject_cast<QLineEdit *>(widget) || qobject_cast<QPlainTextEdit *>(widget))
            return true;
        if (widget->inherits("QWebEngineView"))
            return true;
    }
    return false;
}

} // namespace

void DiffCanvas::copyMarked()
{
    const QString text = markedText();
    if (text.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(text);
}

bool DiffCanvas::copyPress(const QKeyEvent *key)
{
    if (key->isAutoRepeat() || !key->matches(QKeySequence::Copy) || markedText().isEmpty())
        return false;
    if (keepsCopy(QApplication::focusWidget()))
        return false;
    copyMarked();
    return true;
}

void DiffCanvas::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *copy = menu.addAction(QStringLiteral("Copy"));
    copy->setShortcut(QKeySequence::Copy);
    copy->setEnabled(!markedText().isEmpty());
    if (menu.exec(event->globalPos()) == copy)
        copyMarked();
    event->accept();
}
