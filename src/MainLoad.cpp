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

bool sameRef(const QComboBox *box, const QStringList &items, const QString &selected)
{
    if (box->count() != items.size() || box->currentText() != selected)
        return false;
    for (int index = 0; index < items.size(); ++index) {
        if (box->itemText(index) != items.at(index))
            return false;
    }
    return true;
}

void selectRef(QComboBox *box, const QStringList &items, const QString &selected)
{
    if (sameRef(box, items, selected))
        return;
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
    const int scroll = m_quiet ? m_diff->scrollTop() : m_scrollKeep;
    m_title = result.title;
    m_doc = parseDiff(result.diffText);
    m_store->sync(m_doc);
    rebuildTree();
    if (m_doc.files.isEmpty())
        m_diff->setMessage(QStringLiteral("No changes."));
    else {
        m_diff->setDoc(m_doc, result.leftLabel, result.rightLabel);
        m_diff->setScrollTop(scroll);
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

bool MainWindow::keepQuiet(const GitResult &result)
{
    if (result.unchanged)
        return true;
    if (!m_quiet || !result.error.isEmpty() || result.diffText != m_appliedDiff || result.title != m_title)
        return false;
    m_appliedStamp = result.stamp;
    return true;
}

bool MainWindow::stopForError(const GitResult &result)
{
    if (result.error.isEmpty())
        return false;
    if (!m_quiet) {
        m_appliedStamp.clear();
        showLoadError(result);
    }
    finishWatch();
    return true;
}

void MainWindow::onReady(const GitResult &result)
{
    m_loading = false;
    if (!result.root.isEmpty())
        m_seenLoad = true;
    if (keepQuiet(result)) {
        finishWatch();
        return;
    }
    if (stopForError(result))
        return;
    if (!result.root.isEmpty())
        rememberRoot(result.root);
    applyBases(result);
    m_appliedBase = result.baseRef;
    m_appliedHead = result.headRef;
    m_appliedStamp = result.stamp;
    m_appliedDiff = result.diffText;
    showLoadedDiff(result);
    finishWatch();
}
