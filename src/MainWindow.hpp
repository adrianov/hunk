#pragma once

#include "DiffDoc.hpp"

#include <QMainWindow>

class DiffCanvas;
class GitRepo;
class QAction;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QDragEnterEvent;
class QDropEvent;
class QLabel;
class QLineEdit;
class QListWidget;
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
    void fillBar(QToolBar *bar, QAction *openAct, QAction *refreshAct, QAction *commentAct, QAction *copyAct);
    void addModeBox(QToolBar *bar);
    void addBaseBox(QToolBar *bar);
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
    void reloadIfBaseChanged();
    void chooseRepo();
    void onReady(const GitResult &result);
    void rememberRoot(const QString &root);
    void applyBases(const GitResult &result);
    void showLoadError(const GitResult &result);
    void showLoadedDiff(const GitResult &result);
    void showLoadedTitle();
    void rebuildTree();
    void refreshNotes();
    void commentSelection();
    void commentAt(int file, int row, bool oldSide);
    bool commentLine(int file, int row, bool *oldSide, int *line, QString *snippet) const;
    void copyReviews();
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
    QLabel *m_repoLabel = nullptr;
    QListWidget *m_notes = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_delete = nullptr;
    QDockWidget *m_dock = nullptr;
    DiffDoc m_doc;
    QString m_root;
    QString m_startPath;
    QString m_title;
    QString m_appliedBase;
    int m_scrollKeep = 0;
    bool m_noteLock = false;
    bool m_navLock = false;
    bool m_smoke = false;
    bool m_dockSized = false;
};
