// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "GitRepo.hpp"
#include "MdView.hpp"
#include "MainSeen.hpp"
#include "ReviewStore.hpp"

#include <QCheckBox>
#include <QComboBox>
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
    const bool switched = root != m_root;
    m_root = root;
    if (!m_smoke)
        QSettings().setValue(QStringLiteral("lastRepo"), m_root);
    rememberRecent(m_root);
    m_store->setRepo(m_root);
    if (switched)
        loadSeen();
}

void fitRefBox(QComboBox *box);

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
    fitRefBox(box);
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
    m_bodies.clear();
    m_title.clear();
    m_conflict.clear();
    m_drift.clear();
    m_diff->setMessage(result.error);
    m_tree->clear();
    tintRepoLabel();
    if (result.root.isEmpty()) {
        m_repoLabel->setToolTip(result.error);
        m_repoLabel->setText(result.error);
    } else {
        m_repoLabel->setToolTip(result.root);
        m_repoLabel->setText(result.root);
    }
    statusBar()->showMessage(result.error);
    m_md->reload(m_doc, m_root);
    emit loaded(false);
}

void MainWindow::showLoadedDiff(const GitResult &result)
{
    m_title = result.title;
    m_conflict = result.conflict;
    m_drift = result.drift;
    m_doc = result.doc;
    m_bodies.clear();
    applySeen(&m_doc, m_seen);
    m_store->sync(m_doc, m_cleanup->isChecked());
    rebuildTree();
    showDiff(result, m_quiet ? m_diff->scrollTop() : m_scrollKeep);
    m_md->setLabels(result.leftLabel, result.rightLabel);
    m_md->reload(m_doc, m_root);
    keepShownFile();
    pushNoteKeys();
    showLoadedTitle();
}

void MainWindow::showDiff(const GitResult &result, int scroll)
{
    if (m_doc.files.isEmpty()) {
        m_diff->setMessage(QStringLiteral("No changes."));
        return;
    }
    m_diff->setDoc(m_doc, result.leftLabel, result.rightLabel, hiddenIndexes(m_doc, m_hidden));
    m_diff->setScrollTop(scroll);
}

void MainWindow::showLoadedTitle()
{
    showRepoPath();
    setWindowTitle(QStringLiteral("Hunk — ") + m_root);
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

void MainWindow::applyWatch(const GitResult &result)
{
    if (!result.ignoredReady || result.root.isEmpty() || result.root != m_root)
        return;
    armDisk(result.ignored);
}

void MainWindow::onReady(const GitResult &result)
{
    m_loading = false;
    if (!result.root.isEmpty())
        m_seenLoad = true;
    if (!keepQuiet(result)) {
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
    }
    applyWatch(result);
    finishWatch();
}
