#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "GitRepo.hpp"
#include "MainDetail.hpp"
#include "ReviewStore.hpp"

#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QSettings>
#include <QShowEvent>
#include <QStatusBar>
#include <QUrl>

namespace {

void selectMerge(QComboBox *mode)
{
    mode->blockSignals(true);
    mode->setCurrentIndex(static_cast<int>(DiffMode::MergeRequest));
    mode->blockSignals(false);
}

} // namespace

MainWindow::MainWindow(bool smoke, QWidget *parent)
    : QMainWindow(parent)
    , m_smoke(smoke)
{
    applyTheme();
    setWindowTitle(QStringLiteral("Hunk"));
    resize(1280, 800);
    setAcceptDrops(true);
    m_git = new GitRepo(this);
    m_store = new ReviewStore(this);
    wireStore();
    buildChrome();
    buildDiffPane();
    buildReviews();
    restoreWindow();
    wireUi();
}

void MainWindow::wireStore()
{
    connect(m_git, &GitRepo::ready, this, &MainWindow::onReady);
    connect(m_store, &ReviewStore::structureChanged, this, &MainWindow::notesChanged);
    connect(m_store, &ReviewStore::bodyEdited, this, &MainWindow::noteBodyEdited);
}

void MainWindow::notesChanged()
{
    refreshNotes();
    pushNoteKeys();
    updateStatus();
}

void MainWindow::noteBodyEdited(int index)
{
    if (index >= 0 && index < m_notes->count())
        m_notes->item(index)->setText(noteLabel(m_store->notes().at(index)));
    updateStatus();
}

void MainWindow::wireUi()
{
    wireTree();
    wireNotes();
    wireMode();
}

void MainWindow::restoreWindow()
{
    if (m_smoke)
        return;
    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("geometry")).toByteArray());
    restoreState(settings.value(QStringLiteral("windowState")).toByteArray());
    const int mode = settings.value(QStringLiteral("mode"), 0).toInt();
    if (mode >= 0 && mode < m_mode->count())
        m_mode->setCurrentIndex(mode);
}

void MainWindow::reloadFresh()
{
    reload(false);
}

void MainWindow::openStart(const QString &path)
{
    if (!path.isEmpty()) {
        if (!m_smoke)
            selectMerge(m_mode);
        openAt(path);
        return;
    }
    if (m_smoke) {
        openAt(QDir::currentPath());
        return;
    }
    const QString last = QSettings().value(QStringLiteral("lastRepo")).toString();
    openAt(last.isEmpty() ? QDir::currentPath() : last);
}

void MainWindow::openAt(const QString &path)
{
    m_root.clear();
    m_startPath = path;
    m_scrollKeep = 0;
    reload(false);
}

void MainWindow::chooseRepo()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Open repository"),
                                                          m_root.isEmpty() ? m_startPath : m_root);
    if (!dir.isEmpty())
        openAt(dir);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (m_dockSized)
        return;
    m_dockSized = true;
    if (m_smoke || !QSettings().contains(QStringLiteral("windowState")))
        resizeDocks({m_dock}, {200}, Qt::Vertical);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_smoke) {
        QSettings settings;
        settings.setValue(QStringLiteral("geometry"), saveGeometry());
        settings.setValue(QStringLiteral("windowState"), saveState());
        settings.setValue(QStringLiteral("mode"), m_mode->currentIndex());
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty())
        openAt(urls.first().toLocalFile());
}
