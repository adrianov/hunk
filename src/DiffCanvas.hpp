// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "DiffFold.hpp"
#include "DiffPopup.hpp"

#include <QAbstractScrollArea>
#include <QSet>
#include <QTimer>
#include <QVector>

class QContextMenuEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;

enum class SideStyle { Empty, Plain, Add, Del };

// Side-by-side diff. Both panes stay on screen; long lines wrap inside the pane.
// Drag the line between the panes to give one side more room. Double-click it to split them evenly.
class DiffCanvas : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit DiffCanvas(QWidget *parent = nullptr);

    void setMessage(const QString &text);
    void setRoot(const QString &root);
    void setDoc(const DiffDoc &doc, const QString &leftLabel, const QString &rightLabel, const QSet<int> &skip);
    void setSkipped(const QSet<int> &files);
    void setNoteKeys(const QSet<QString> &keys);
    void setSearch(const QString &text);
    void showFile(int file);
    void showRow(int file, int row, bool oldSide);
    int scrollTop() const;
    void setScrollTop(int y);
    int headerHeight() const { return m_headerH; }
    void barFile(int file);
    void markReviewed(int file) { if (file >= 0) emit reviewRequested(file); }
    bool hasSelection() const { return m_selFile >= 0 && m_selRow >= 0; }
    int selectedFile() const { return m_selFile; }
    int selectedRow() const { return m_selRow; }
    bool selectedOldSide() const { return m_selOld; }
    bool markedRows(int file, int row, bool oldSide, int *from, int *to) const;

signals:
    void commentRequested(int file, int row, bool oldSide);
    void reviewRequested(int file);
    void fileScrolled(int file);
    void openDefinition(const QString &path, int line);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    struct Band {
        enum Kind { Header, Labels, Row, Note, Fold };
        Kind kind = Row;
        int file = -1;
        int row = -1;
        int y = 0;
        int h = 0;
        int end = -1;
    };

    struct TextMark {
        int file = -1;
        int row = -1;
        bool old = false;
        int pos = 0;
        bool code = false;
    };

    void ensureFont();
    void prepareWidth();
    int clampSplit(int viewW, int x) const;
    int pairSplit(int viewW) const;
    bool layoutStale() const;
    int sideOrigin(bool single, int x, bool *left) const;
    void dropPieces();
    void rebuild();
    void relayoutFile(int fileIndex);
    int fileBands(int fileIndex, int *end) const;
    void appendTail(const QList<Band> &tail, int delta);
    void publishLayout();
    void applyNotePaths(const QSet<QString> &keys);
    void addFileBands(int fileIndex, int *y);
    void addRowBands(int fileIndex, int *y);
    void addFoldBand(int fileIndex, const FoldSpan &span, int *y);
    void addCodeBand(int fileIndex, int rowIndex, bool single, int *y);
    QSet<int> pinnedRows(int fileIndex) const;
    void showHidden(int file, int first, int last);
    void revealRow(int file, int row);
    void ensureRow(int fileIndex, int rowIndex, bool single);
    void wrapRow(DiffRow *row, bool single);
    int rowHeight(const DiffRow &row, bool single) const;
    void updateScroll();
    void updateVertical(int viewH);
    void paintContents();
    void paintDoc(QPainter &painter);
    void paintVisible(QPainter &painter, int scrollY);
    void paintSticky(QPainter &painter, int scrollY);
    void paintBand(QPainter &painter, const Band &band);
    void paintHeaderBand(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintLabels(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintNoteBand(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintRow(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintFold(QPainter &painter, const Band &band, int viewW);
    void paintSingle(QPainter &painter, const Band &band, const FileDiff &file, const DiffRow &row, int viewW);
    void paintPair(QPainter &painter, const Band &band, const FileDiff &file, const DiffRow &row, int viewW);
    void paintSelection(QPainter &painter, const Band &band, int viewW);
    void paintOneSide(QPainter &painter, int cellX, int cellW, SideStyle style, RowHeat heat, const QString &text,
                      int number, const QList<Piece> &pieces, bool note, int top, int height, int file, int row,
                      bool oldSide);
    void paintTextMark(QPainter &painter, int cellX, int cellW, int top, int height, int file, int row, bool oldSide,
                       const QString &text, const QList<Piece> &pieces);
    void paintSearch(QPainter &painter, int cellX, int cellW, int top, int height, const QString &text,
                     const QList<Piece> &pieces);
    void paintFileHeader(QPainter &painter, const QRect &rect, const FileDiff &file, const QFont &font, bool hot);
    QRect titleCheck(const QRect &header) const;
    bool stickyZone(int viewY, int scrollY) const;
    int stuckTitle(int scrollY, QRect *check) const;
    int bandTitle(int viewY, int scrollY, QRect *check) const;
    int titleFile(int viewY, QRect *check) const;
    bool titlePress(int x, int viewY);
    bool hoverTitle(int x, int viewY);
    void takePress(QMouseEvent *mouse);
    void trackHover(QMouseEvent *mouse, int x, int y);
    bool pressIgnored(QMouseEvent *mouse) const;
    bool headerPress(QMouseEvent *mouse) const;
    bool onSplit(const Band *band, int x) const;
    bool grabSplit(const Band *band, int x);
    bool dragSplit(int x, QMouseEvent *mouse);
    void moveSplit(int x);
    void hoverMouse(QMouseEvent *mouse);
    void commitSplit();
    bool releaseSplit(QMouseEvent *mouse);
    bool evenSplit(QMouseEvent *mouse);
    void chooseRow(const Band &band, int x);
    void hoverRow(const Band *band);
    void hoverCursor(const Band *band, int x);
    void placeCursor();
    bool canFollow() const;
    void followHand();
    void seek(const QString &name);
    void afterSeek(const QString &name);
    bool seekBusy(const QString &name) const;
    bool awaitSeek();
    bool deferJump(const QString &name, const QString &path, int line, int column, const QString &receiver);
    void armHover();
    void onScroll();
    QString methodName(int file, int row, bool old, int column, int *line, QString *receiver) const;
    QString linkPath(const TextMark &hit) const;
    void hoverDef(int x, int y);
    void keepDef(const QString &name, const QString &path, int line, int column, const QString &receiver);
    void showDef();
    bool jumpMod(Qt::KeyboardModifiers mods) const;
    void chooseDef(int x, int y);
    bool openSpot(const QString &path, int line, int column, const QString &name, const QString &receiver);
    void holdJump(const QString &name, const QString &path, int line, int column, const QString &receiver);
    void finishJump();
    bool mouseEvent(QEvent *event);
    void dropDef();
    void stopDef();
    bool overDef() const;
    DefPopup *ensurePopup();
    bool openFold(const Band *band, int x);
    void setHover(int file, int row);
    void onPress(QMouseEvent *mouse);
    void pressAt(QMouseEvent *mouse, int x, int y);
    void onMove(QMouseEvent *mouse);
    void pressRow(const Band &band, int x, int y, QMouseEvent *mouse);
    void clearMark();
    void beginText(int x, int y, bool extend);
    TextMark codeAt(TextMark mark, const DiffRow &row, int localX, int localY) const;
    void takeWord(const TextMark &hit);
    bool wordFresh() const;
    bool onWord(const TextMark &hit) const;
    bool lineMarked(const TextMark &hit) const;
    void markLine(const TextMark &hit, int size);
    TextMark clickMark(const QMouseEvent *mouse) const;
    bool takeLine(int x, int y);
    void dragText(int x, int y);
    void moveDrag(int x, int y);
    void armDrag();
    void scrollDrag();
    void trackDrag(int x, int viewY);
    bool dragHeld();
    int textTop() const;
    int outsideAt(int viewY) const;
    bool rollDrag(int outside);
    void paceDrag(int outside);
    int edgeDoc(int viewY) const;
    int paneX(int x) const;
    void dragEdge(int x, int y, bool upward);
    void endText(QMouseEvent *mouse);
    void selectAll();
    int paneFile() const;
    bool paneOld(int file) const;
    bool sideEnd(int file, bool old, int *first, int *last, int *end) const;
    bool fullMark(const TextMark &from, const TextMark &to) const;
    void dropDrag();
    void showMark(int file, bool old, int first, int last, int end);
    bool paneKey(const QKeyEvent *key);
    bool paneFocus() const;
    bool selectPress(const QKeyEvent *key);
    bool movePress(const QKeyEvent *key);
    bool placeCaret();
    void viewEdge(int *top, int *bottom) const;
    bool rowShown(int file, int row) const;
    int shownRow(int file, bool old, int first, int last) const;
    int caretRow(int file, bool old, int first, int last) const;
    void seedCaret(int file, bool old, int row);
    bool collapseMove(bool forward);
    void moveCaret(int kind, bool extend);
    void applyMove(int kind);
    void stepChar(int dir);
    void stepWord(int dir);
    void stepLine(int dir);
    void stepEdge(bool end);
    void stepDoc(bool end);
    int nearRow(int row, int dir) const;
    QString caretLine() const;
    void followCaret();
    void scrollCaret();
    const Band *caretBand() const;
    void scrollBand(const Band &band);
    void pickWord(QMouseEvent *mouse);
    TextMark textAt(int x, int y) const;
    bool markEnds(TextMark *from, TextMark *to) const;
    bool markSpan(int file, int row, bool oldSide, int size, int *start, int *end) const;
    QString markedText() const;
    void copyMarked();
    bool copyPress(const QKeyEvent *key);
    void clearHover();
    const Band *bandAt(int y) const;
    bool inGutter(const FileDiff &file, int x, bool *oldSide) const;
    int pointFile() const;
    void emitVisibleFile();
    int fileAt(int y) const;
    bool barHeads(int file) const { return file == m_barFile; }
    bool headerStuck(int scrollY) const;

    DiffDoc m_doc;
    QString m_root;
    QString m_leftLabel;
    QString m_rightLabel;
    QString m_message;
    QString m_search;
    QList<Band> m_bands;
    QSet<QString> m_notes;
    QSet<int> m_skip;
    int m_barFile = -1;
    QVector<QSet<int>> m_open;
    QVector<int> m_wrapW;
    QFont m_mono;
    int m_rowH = 20;
    int m_headerH = 34;
    int m_labelH = 24;
    int m_gutterW = 56;
    int m_docH = 0;
    int m_viewW = -1;
    double m_split = 0.5;
    int m_splitW = -1;
    bool m_dragSplit = false;
    int m_splitDx = 0;
    QTimer m_wrapTimer;
    int m_selFile = -1;
    int m_selRow = -1;
    bool m_selOld = false;
    TextMark m_anchor;
    TextMark m_caret;
    int m_goal = -1;
    qint64 m_wordMs = 0;
    bool m_dragText = false;
    QTimer m_dragTimer;
    int m_hoverFile = -1;
    int m_hoverRow = -1;
    QTimer m_defTimer;
    QString m_defName;
    QSet<QString> m_seeking;
    QString m_defPath;
    int m_defLine = 0;
    int m_defColumn = 0;
    QString m_defReceiver;
    bool m_defWait = false;
    QString m_link;
    QString m_jumpName;
    QString m_jumpPath;
    int m_jumpLine = 0;
    int m_jumpColumn = 0;
    QString m_jumpReceiver;
    QWidget *m_defPopup = nullptr;
    int m_checkFile = -1;
    int m_lastFile = -1;
};
