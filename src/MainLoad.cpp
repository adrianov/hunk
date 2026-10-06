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

void selectBase(QComboBox *base, const GitResult &result)
{
    const QString typed = base->currentText();
    base->clear();
    base->addItems(result.bases);
    if (!result.baseRef.isEmpty())
        base->setCurrentText(result.baseRef);
    else if (!typed.isEmpty())
        base->setCurrentText(typed);
}

} // namespace

void MainWindow::applyBases(const GitResult &result)
{
    setBaseBlocked(m_base, true);
    selectBase(m_base, result);
    setBaseBlocked(m_base, false);
    m_base->setEnabled(static_cast<DiffMode>(m_mode->currentIndex()) == DiffMode::MergeRequest);
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
    m_appliedBase = result.baseRef;
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
    if (!result.error.isEmpty())
        showLoadError(result);
    else
        showLoadedDiff(result);
}
