#include "MainWindow.hpp"

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
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

void addRef(QToolBar *bar, QComboBox **box, const QString &label, const QString &tip)
{
    bar->addWidget(new QLabel(label, bar));
    *box = new QComboBox(bar);
    (*box)->setEditable(true);
    (*box)->setMinimumWidth(160);
    (*box)->setToolTip(tip);
    bar->addWidget(*box);
}

void MainWindow::addRefBoxes(QToolBar *bar)
{
    addRef(bar, &m_base, QStringLiteral("Base"),
           QStringLiteral("Base of the merge request. Branching point is where this branch left the default branch."));
    addRef(bar, &m_head, QStringLiteral("Branch"), QStringLiteral("Branch compared with the base"));
}

void MainWindow::fillBar(QToolBar *bar, QAction *openAct, QAction *refreshAct, QAction *commentAct, QAction *copyAct)
{
    bar->addAction(openAct);
    addModeBox(bar);
    addRefBoxes(bar);
    bar->addAction(refreshAct);
    bar->addAction(commentAct);
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
    auto *bar = addToolBar(QStringLiteral("Main"));
    bar->setMovable(false);
    fillBar(bar, openAct, refreshAct, commentAct, copyAct);
}
