// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "MainRef.hpp"

#include <QGuiApplication>
#include <QLabel>
#include <QSizePolicy>
#include <QToolBar>

void useTextColor(QWidget *widget)
{
    if (!widget)
        return;
    const QColor text = qApp->palette().color(QPalette::Active, QPalette::Text);
    QPalette palette = widget->palette();
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::ButtonText, text);
    widget->setForegroundRole(QPalette::WindowText);
    widget->setPalette(palette);
}

namespace {

void tintToolbar(QWidget *widget)
{
    auto *bar = qobject_cast<QToolBar *>(widget ? widget->parentWidget() : nullptr);
    if (!bar)
        return;
    for (QLabel *label : bar->findChildren<QLabel *>(Qt::FindDirectChildrenOnly))
        useTextColor(label);
}

} // namespace

void MainWindow::prepareRepoLabel()
{
    m_repoLabel->setMinimumWidth(0);
    m_repoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_repoLabel->installEventFilter(this);
    tintRepoLabel();
}

void MainWindow::tintRepoLabel()
{
    useTextColor(m_repoLabel);
    tintToolbar(m_repoLabel);
    if (m_mode)
        showRefText(m_mode);
    if (m_base)
        showRefText(m_base);
    if (m_head)
        showRefText(m_head);
}

void MainWindow::showRepoPath()
{
    if (!m_repoLabel)
        return;
    tintRepoLabel();
    if (m_root.isEmpty())
        return;
    m_repoLabel->setToolTip(m_root);
    const int width = qMax(0, m_repoLabel->width() - 8);
    if (width < 12)
        return;
    const QString shown = m_repoLabel->fontMetrics().elidedText(m_root, Qt::ElideMiddle, width);
    if (m_repoLabel->text() != shown)
        m_repoLabel->setText(shown);
}
