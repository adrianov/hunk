#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "ReviewStore.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

QWidget *MainWindow::makeFilePane()
{
    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(QStringLiteral("Filter files"));
    m_filter->setClearButtonEnabled(true);
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setIndentation(14);
    auto *left = new QWidget(this);
    auto *layout = new QVBoxLayout(left);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_filter);
    layout->addWidget(m_tree, 1);
    return left;
}

void MainWindow::buildDiffPane()
{
    m_diff = new DiffCanvas(this);
    auto *split = new QSplitter(this);
    split->addWidget(makeFilePane());
    split->addWidget(m_diff);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({280, 1000});
    setCentralWidget(split);
}

namespace {

QPlainTextEdit *makeEditor(QWidget *parent)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setPlaceholderText(QStringLiteral("Click a line number in the diff, then write a review comment."));
    editor->setEnabled(false);
    return editor;
}

QCheckBox *makeCleanup(QWidget *parent, bool smoke)
{
    auto *box = new QCheckBox(QStringLiteral("Auto cleanup reviews for changed lines"), parent);
    box->setChecked(smoke || QSettings().value(QStringLiteral("autoCleanup"), true).toBool());
    return box;
}

QHBoxLayout *reviewButtons(QPushButton *copy, QCheckBox *cleanup)
{
    auto *buttons = new QHBoxLayout();
    buttons->addWidget(copy);
    buttons->addWidget(cleanup);
    buttons->addStretch();
    return buttons;
}

} // namespace

QWidget *MainWindow::reviewPanel()
{
    m_notes = new QListWidget(this);
    m_editor = makeEditor(this);
    m_copy = new QPushButton(QStringLiteral("Copy reviews"), this);
    m_cleanup = makeCleanup(this, m_smoke);
    auto *panel = new QWidget(this);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_notes, 1);
    layout->addWidget(m_editor, 1);
    layout->addLayout(reviewButtons(m_copy, m_cleanup));
    return panel;
}

void MainWindow::buildReviews()
{
    m_dock = new QDockWidget(QStringLiteral("Reviews"), this);
    m_dock->setWidget(reviewPanel());
    addDockWidget(Qt::BottomDockWidgetArea, m_dock);
    menuBar()->addMenu(QStringLiteral("View"))->addAction(m_dock->toggleViewAction());
}

void MainWindow::openTreeItem(QTreeWidgetItem *item)
{
    if (!item)
        return;
    const int index = item->data(0, Qt::UserRole).toInt();
    if (index < 0)
        return;
    m_navLock = true;
    m_diff->showFile(index);
    m_navLock = false;
}

void MainWindow::wireTree()
{
    connect(m_filter, &QLineEdit::textChanged, this, &MainWindow::rebuildTree);
    connect(m_tree, &QTreeWidget::itemClicked, this, &MainWindow::openTreeItem);
    connect(m_diff, &DiffCanvas::fileScrolled, this, &MainWindow::selectTreeFile);
    connect(m_diff, &DiffCanvas::commentRequested, this, &MainWindow::commentAt);
}

void MainWindow::wireNotes()
{
    connect(m_notes, &QListWidget::currentRowChanged, this, &MainWindow::showNote);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::saveNote);
    connect(m_copy, &QPushButton::clicked, this, &MainWindow::copyReviews);
    connect(m_cleanup, &QCheckBox::toggled, this, [this](bool on) {
        if (!m_smoke)
            QSettings().setValue(QStringLiteral("autoCleanup"), on);
        m_store->sync(m_doc, on);
    });
}

void MainWindow::wireMode()
{
    connect(m_mode, &QComboBox::currentIndexChanged, this, &MainWindow::reloadFresh);
    connect(m_base, &QComboBox::activated, this, &MainWindow::reloadIfRangeChanged);
    connect(m_head, &QComboBox::activated, this, &MainWindow::reloadIfRangeChanged);
    if (m_base->lineEdit())
        connect(m_base->lineEdit(), &QLineEdit::editingFinished, this, &MainWindow::reloadIfRangeChanged);
    if (m_head->lineEdit())
        connect(m_head->lineEdit(), &QLineEdit::editingFinished, this, &MainWindow::reloadIfRangeChanged);
}
