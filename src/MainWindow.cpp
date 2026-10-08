#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "GitRepo.hpp"
#include "MainDetail.hpp"
#include "ReviewStore.hpp"

#include <QCloseEvent>
#include <QGuiApplication>
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
    noteSystemTheme();
    applyTheme(m_smoke);
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
    watchSystemTheme();
}

void MainWindow::wireStore()
{
    m_watch.setSingleShot(true);
    m_watch.setInterval(400);
    connect(&m_watch, &QTimer::timeout, this, &MainWindow::reloadQuiet);
    connect(&m_disk, &QFileSystemWatcher::directoryChanged, this, &MainWindow::noteDisk);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::watchApp);
    connect(m_git, &GitRepo::ready, this, &MainWindow::onReady);
    connect(m_store, &ReviewStore::structureChanged, this, &MainWindow::notesChanged);
    connect(m_store, &ReviewStore::bodyEdited, this, &MainWindow::noteBodyEdited);
}

void MainWindow::watchApp(Qt::ApplicationState state)
{
    if (state != Qt::ApplicationActive) {
        m_watch.stop();
        return;
    }
    if (m_seenLoad)
        m_appliedStamp.clear();
    reloadQuiet();
}

void MainWindow::notesChanged()
{
    refreshNotes();
    pushNoteKeys();
    updateStatus();
}

void MainWindow::noteBodyEdited(int index)
{
    if (index < 0 || index >= m_notes->count() || index >= m_store->notes().size())
        return;
    if (QWidget *row = m_notes->itemWidget(m_notes->item(index))) {
        if (auto *label = row->findChild<QLabel *>())
            label->setText(noteLabel(m_store->notes().at(index)));
    }
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
    m_appliedStamp.clear();
    m_appliedDiff.clear();
    m_title.clear();
    m_watch.stop();
    clearDisk();
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
