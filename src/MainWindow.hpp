// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "MainDetail.hpp"

#include <QFileSystemWatcher>
#include <QHash>
#include <QMainWindow>
#include <QSet>
#include <QTimer>

class DiffCanvas;
class GitRepo;
class QAction;
class QActionGroup;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QShowEvent;
class QToolBar;
class QTreeWidget;
class QTreeWidgetItem;
class ReviewStore;
struct GitResult;

// Merge-request window: file list, side-by-side diff, and review comments.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(bool smoke = false, QWidget *parent = nullptr);
    void openStart(const QString &path);

signals:
    void loaded(bool ok);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    bool noteEvents(QObject *object, QEvent *event);

private:
    void wireStore();
    void notesChanged();
    void noteBodyEdited(int index);
    void buildChrome();
    QAction *makeOpen();
    QAction *makeRefresh();
    QAction *makeComment();
    QAction *makeCopy();
    QAction *makeQuit();
    void addMenus(QAction *openAct, QAction *refreshAct, QAction *quitAct, QAction *commentAct, QAction *copyAct);
    void addThemeMenu(QMenu *view);
    void addThemeAction(QMenu *menu, QActionGroup *group, ThemePick pick, const QString &label);
    void chooseTheme(ThemePick pick);
    void repaintTheme();
    void watchSystemTheme();
    void fillBar(QToolBar *bar, QAction *openAct, QAction *refreshAct, QAction *commentAct, QAction *copyAct);
    void addModeBox(QToolBar *bar);
    void addRefBoxes(QToolBar *bar);
    void buildDiffPane();
    QWidget *makeFilePane();
    void buildReviews();
    QWidget *reviewPanel();
    void wireUi();
    void wireTree();
    void wireNotes();
    void wireMode();
    void restoreWindow();
    void openTreeItem(QTreeWidgetItem *item);
    void showNote(int row);
    void loadNote(int row);
    void saveNote();
    void reloadFresh();
    void openAt(const QString &path);
    void reload(bool keepScroll);
    void reloadQuiet();
    void askLoad(bool keepScroll, bool quiet);
    void startLoad(const QString &path);
    void scheduleWatch(int msec = 400);
    void finishWatch();
    void watchApp(Qt::ApplicationState state);
    void clearDisk();
    void armDisk(const QStringList &ignored);
    void noteDisk(const QString &path);
    void markGap();
    bool growDisk(const QString &dir, const QString &skip);
    bool watchTree();
    bool watchRefs();
    void applyWatch(const GitResult &result);
    void reloadIfRangeChanged();
    void chooseRepo();
    void onReady(const GitResult &result);
    bool keepQuiet(const GitResult &result);
    bool stopForError(const GitResult &result);
    void rememberRoot(const QString &root);
    void applyBases(const GitResult &result);
    void showLoadError(const GitResult &result);
    void showLoadedDiff(const GitResult &result);
    void showLoadedTitle();
    void prepareRepoLabel();
    void showRepoPath();
    void tintRepoLabel();
    void rebuildTree();
    void loadSeen();
    void markSeenFile(int file);
    void fillNotes();
    void refreshNotes();
    int keptNote(int slot, int oldCount, const QString &path, bool oldSide, int line, const QString &body, bool editing);
    void placeNoteRows();
    void addListedNote(int index);
    void restoreNoteRow(int row, bool editing);
    void commentSelection();
    void commentAt(int file, int row, bool oldSide);
    bool commentLine(int file, int row, bool *oldSide, int *line, QString *snippet) const;
    void copyReviews();
    void showStatus(const QString &text);
    void showLineStat();
    void updateStatus();
    void pushNoteKeys();
    void selectTreeFile(int file);

    GitRepo *m_git = nullptr;
    ReviewStore *m_store = nullptr;
    DiffCanvas *m_diff = nullptr;
    QTreeWidget *m_tree = nullptr;
    QLineEdit *m_filter = nullptr;
    QComboBox *m_mode = nullptr;
    QComboBox *m_base = nullptr;
    QComboBox *m_head = nullptr;
    QLabel *m_repoLabel = nullptr;
    QLabel *m_stat = nullptr;
    QListWidget *m_notes = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_copy = nullptr;
    QCheckBox *m_cleanup = nullptr;
    QDockWidget *m_dock = nullptr;
    DiffDoc m_doc;
    QHash<QString, QString> m_seen;
    QString m_root;
    QString m_startPath;
    QString m_title;
    QString m_conflict;
    QString m_drift;
    QString m_appliedBase;
    QString m_appliedHead;
    QString m_appliedStamp;
    QString m_appliedDiff;
    QTimer m_watch;
    QFileSystemWatcher m_disk;
    QString m_gitDir;
    QString m_watchRoot;
    QSet<QString> m_ignored;
    bool m_diskGap = false;
    int m_gapLeft = 0;
    int m_scrollKeep = 0;
    bool m_loading = false;
    bool m_quiet = false;
    bool m_seenLoad = false;
    bool m_watchAgain = false;
    bool m_noteLock = false;
    bool m_navLock = false;
    bool m_smoke = false;
    bool m_dockSized = false;
};
