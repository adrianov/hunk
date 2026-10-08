// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "MainDetail.hpp"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QToolBar>

QAction *MainWindow::makeOpen()
{
    auto *action = new QAction(QStringLiteral("Open"), this);
    action->setShortcut(QKeySequence::Open);
    connect(action, &QAction::triggered, this, &MainWindow::chooseRepo);
    return action;
}

QAction *MainWindow::makeRefresh()
{
    auto *action = new QAction(QStringLiteral("Refresh"), this);
    action->setShortcut(QKeySequence::Refresh);
    connect(action, &QAction::triggered, this, [this]() { reload(true); });
    return action;
}

QAction *MainWindow::makeComment()
{
    auto *action = new QAction(QStringLiteral("Comment"), this);
    action->setShortcut(Qt::CTRL | Qt::Key_Return);
    action->setShortcutContext(Qt::WindowShortcut);
    action->setToolTip(QStringLiteral("Comment on the selected line"));
    connect(action, &QAction::triggered, this, &MainWindow::commentSelection);
    return action;
}

QAction *MainWindow::makeCopy()
{
    auto *action = new QAction(QStringLiteral("Copy reviews"), this);
    action->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_C);
    action->setShortcutContext(Qt::WindowShortcut);
    action->setToolTip(QStringLiteral("Copy review comments for an LLM agent"));
    connect(action, &QAction::triggered, this, &MainWindow::copyReviews);
    return action;
}

QAction *MainWindow::makeQuit()
{
    auto *action = new QAction(QStringLiteral("Quit"), this);
    action->setMenuRole(QAction::QuitRole);
    action->setShortcut(QKeySequence::Quit);
    connect(action, &QAction::triggered, this, &QWidget::close);
    return action;
}

void MainWindow::addThemeAction(QMenu *menu, QActionGroup *group, ThemePick pick, const QString &label)
{
    auto *action = menu->addAction(label);
    action->setCheckable(true);
    action->setChecked(pick == themePick());
    group->addAction(action);
    connect(action, &QAction::triggered, this, [this, pick]() { chooseTheme(pick); });
}

void MainWindow::addThemeMenu(QMenu *view)
{
    auto *menu = view->addMenu(QStringLiteral("Theme"));
    auto *group = new QActionGroup(menu);
    group->setExclusive(true);
    addThemeAction(menu, group, ThemePick::System, QStringLiteral("System"));
    addThemeAction(menu, group, ThemePick::Dark, QStringLiteral("Dark"));
    addThemeAction(menu, group, ThemePick::Light, QStringLiteral("Light"));
}

void MainWindow::addMenus(QAction *openAct, QAction *refreshAct, QAction *quitAct, QAction *commentAct, QAction *copyAct)
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(refreshAct);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAct);
    auto *reviewMenu = menuBar()->addMenu(QStringLiteral("Review"));
    reviewMenu->addAction(commentAct);
    reviewMenu->addAction(copyAct);
}

void MainWindow::addModeBox(QToolBar *bar)
{
    m_mode = new QComboBox(this);
    m_mode->addItem(QStringLiteral("Merge request"));
    m_mode->addItem(QStringLiteral("Uncommitted"));
    m_mode->addItem(QStringLiteral("Staged"));
    m_mode->setItemData(0, QStringLiteral("Three-dot diff from the base, including uncommitted changes"), Qt::ToolTipRole);
    m_mode->setItemData(1, QStringLiteral("Staged and unstaged changes against HEAD"), Qt::ToolTipRole);
    m_mode->setItemData(2, QStringLiteral("Staged changes only"), Qt::ToolTipRole);
    bar->addWidget(m_mode);
}

void applyBarButton(QPushButton *button, const QAction *action)
{
    button->setText(action->text());
    button->setToolTip(action->toolTip().isEmpty() ? action->text() : action->toolTip());
    button->setEnabled(action->isEnabled());
    button->setVisible(action->isVisible());
    button->setCheckable(action->isCheckable());
    button->setChecked(action->isChecked());
}

void addBarButton(QToolBar *bar, QAction *action)
{
    auto *button = new QPushButton(bar);
    applyBarButton(button, action);
    QObject::connect(button, &QPushButton::clicked, action, &QAction::trigger);
    QObject::connect(action, &QAction::changed, button, [button, action]() { applyBarButton(button, action); });
    bar->addWidget(button);
}

void MainWindow::fillBar(QToolBar *bar, QAction *openAct, QAction *refreshAct, QAction *commentAct, QAction *copyAct)
{
    addBarButton(bar, openAct);
    addModeBox(bar);
    addRefBoxes(bar);
    addBarButton(bar, refreshAct);
    addBarButton(bar, commentAct);
    bar->addSeparator();
    bar->addWidget(m_repoLabel);
    addAction(commentAct);
    addAction(copyAct);
}

void MainWindow::buildChrome()
{
    auto *openAct = makeOpen();
    auto *refreshAct = makeRefresh();
    auto *commentAct = makeComment();
    auto *copyAct = makeCopy();
    addMenus(openAct, refreshAct, makeQuit(), commentAct, copyAct);
    m_repoLabel = new QLabel(QStringLiteral("No repository"), this);
    prepareRepoLabel();
    auto *bar = addToolBar(QStringLiteral("Main"));
    bar->setMovable(false);
    fillBar(bar, openAct, refreshAct, commentAct, copyAct);
}
