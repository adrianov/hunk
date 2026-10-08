#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "DiffParse.hpp"
#include "GitRepo.hpp"
#include "ReviewStore.hpp"

#include <QComboBox>
#include <QDir>
#include <QSettings>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <QTreeWidget>

void MainWindow::startLoad(const QString &path)
{
    const auto mode = static_cast<DiffMode>(m_mode->currentIndex());
    const bool merge = mode == DiffMode::MergeRequest;
    m_base->setEnabled(merge);
    m_head->setEnabled(merge);
    m_git->load(path, mode, m_base->currentText().trimmed(), m_head->currentText().trimmed());
}

void MainWindow::reload(bool keepScroll)
{
    const QString path = m_root.isEmpty() ? m_startPath : m_root;
    if (path.isEmpty())
        return;
    m_scrollKeep = keepScroll ? m_diff->scrollTop() : 0;
    m_repoLabel->setText(QStringLiteral("Loading…"));
    m_diff->setMessage(QStringLiteral("Loading…"));
    statusBar()->showMessage(QStringLiteral("Loading…"));
    startLoad(path);
}

void MainWindow::reloadIfRangeChanged()
{
    if (m_base->currentText().trimmed() == m_appliedBase && m_head->currentText().trimmed() == m_appliedHead)
        return;
    reload(false);
}

void MainWindow::rememberRoot(const QString &root)
{
    m_root = root;
    if (!m_smoke)
        QSettings().setValue(QStringLiteral("lastRepo"), m_root);
    m_store->setRepo(m_root);
}

namespace {

void setBaseBlocked(QComboBox *base, bool blocked)
{
    base->blockSignals(blocked);
    if (base->lineEdit())
        base->lineEdit()->blockSignals(blocked);
}

void selectRef(QComboBox *box, const QStringList &items, const QString &selected)
{
    const QString typed = box->currentText();
    box->clear();
    box->addItems(items);
    if (!selected.isEmpty())
        box->setCurrentText(selected);
    else if (!typed.isEmpty())
        box->setCurrentText(typed);
}

} // namespace

void MainWindow::applyBases(const GitResult &result)
{
    setBaseBlocked(m_base, true);
    setBaseBlocked(m_head, true);
    selectRef(m_base, result.bases, result.baseRef);
    selectRef(m_head, result.branches, result.headRef);
    setBaseBlocked(m_base, false);
    setBaseBlocked(m_head, false);
    const bool merge = static_cast<DiffMode>(m_mode->currentIndex()) == DiffMode::MergeRequest;
    m_base->setEnabled(merge);
    m_head->setEnabled(merge);
}

void MainWindow::showLoadError(const GitResult &result)
{
    m_doc = {};
    m_title.clear();
    m_diff->setMessage(result.error);
    m_tree->clear();
    m_repoLabel->setText(result.root.isEmpty() ? result.error : QDir(result.root).dirName());
    statusBar()->showMessage(result.error);
    emit loaded(false);
}

void MainWindow::showLoadedDiff(const GitResult &result)
{
    m_title = result.title;
    m_doc = parseDiff(result.diffText);
    m_store->sync(m_doc);
    rebuildTree();
    if (m_doc.files.isEmpty())
        m_diff->setMessage(QStringLiteral("No changes."));
    else {
        m_diff->setDoc(m_doc, result.leftLabel, result.rightLabel);
        m_diff->setScrollTop(m_scrollKeep);
    }
    pushNoteKeys();
    showLoadedTitle();
}

void MainWindow::showLoadedTitle()
{
    const QString name = QDir(m_root).dirName();
    m_repoLabel->setText(name + QStringLiteral("    ") + m_title);
    setWindowTitle(QStringLiteral("Hunk — ") + name);
    updateStatus();
    emit loaded(true);
}

void MainWindow::onReady(const GitResult &result)
{
    if (!result.root.isEmpty())
        rememberRoot(result.root);
    applyBases(result);
    m_appliedBase = result.baseRef;
    m_appliedHead = result.headRef;
    if (!result.error.isEmpty())
        showLoadError(result);
    else
        showLoadedDiff(result);
}
