#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "GitRepo.hpp"

#include <QComboBox>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>

namespace {

bool editingRef(const QComboBox *box)
{
    return box->hasFocus() || (box->lineEdit() && box->lineEdit()->hasFocus());
}

} // namespace

void MainWindow::startLoad(const QString &path)
{
    const auto mode = static_cast<DiffMode>(m_mode->currentIndex());
    const bool merge = mode == DiffMode::MergeRequest;
    m_base->setEnabled(merge);
    m_head->setEnabled(merge);
    m_loading = true;
    m_git->load(path, mode, m_base->currentText().trimmed(), m_head->currentText().trimmed(), m_appliedStamp, m_quiet);
}

void MainWindow::askLoad(bool keepScroll, bool quiet)
{
    const QString path = m_root.isEmpty() ? m_startPath : m_root;
    if (path.isEmpty())
        return;
    m_scrollKeep = keepScroll ? m_diff->scrollTop() : 0;
    m_quiet = quiet;
    if (!quiet) {
        m_repoLabel->setText(QStringLiteral("Loading…"));
        m_diff->setMessage(QStringLiteral("Loading…"));
        statusBar()->showMessage(QStringLiteral("Loading…"));
    }
    startLoad(path);
}

void MainWindow::reload(bool keepScroll)
{
    m_watch.stop();
    askLoad(keepScroll, false);
}

void MainWindow::reloadQuiet()
{
    if (m_smoke || !m_seenLoad)
        return;
    if (editingRef(m_base) || editingRef(m_head)) {
        scheduleWatch();
        return;
    }
    if (m_loading) {
        m_watchAgain = true;
        return;
    }
    askLoad(true, true);
}

void MainWindow::scheduleWatch()
{
    if (m_smoke || m_root.isEmpty() || QGuiApplication::applicationState() != Qt::ApplicationActive)
        return;
    m_watch.start();
}

void MainWindow::finishWatch()
{
    if (!m_watchAgain) {
        scheduleWatch();
        return;
    }
    m_watchAgain = false;
    reloadQuiet();
}
