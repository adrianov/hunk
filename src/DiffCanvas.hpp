#pragma once

#include "DiffDoc.hpp"

#include <QAbstractScrollArea>
#include <QSet>

class QMouseEvent;
class QPainter;

enum class SideStyle { Empty, Plain, Add, Del };

// Side-by-side diff. Both panes stay on screen; long lines scroll horizontally.
class DiffCanvas : public QAbstractScrollArea {
    Q_OBJECT
public:
    explicit DiffCanvas(QWidget *parent = nullptr);

    void setMessage(const QString &text);
    void setDoc(const DiffDoc &doc, const QString &leftLabel, const QString &rightLabel);
    void setNoteKeys(const QSet<QString> &keys);
    void showFile(int file);
    void showRow(int file, int row, bool oldSide);
    int scrollTop() const;
    void setScrollTop(int y);
    bool hasSelection() const { return m_selFile >= 0 && m_selRow >= 0; }
    int selectedFile() const { return m_selFile; }
    int selectedRow() const { return m_selRow; }
    bool selectedOldSide() const { return m_selOld; }

signals:
    void commentRequested(int file, int row, bool oldSide);
    void fileScrolled(int file);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool viewportEvent(QEvent *event) override;

private:
    struct Band {
        enum Kind { Header, Labels, Row, Note };
        Kind kind = Row;
        int file = -1;
        int row = -1;
        int y = 0;
        int h = 0;
    };

    void ensureFont();
    void rebuild();
    void addFileBands(int fileIndex, int *y);
    void trackLine(const QString &text);
    void updateScroll();
    void updateVertical(int viewH);
    void updateHorizontal();
    void paintContents();
    void paintDoc(QPainter &painter);
    void paintVisible(QPainter &painter, int scrollX, int scrollY);
    void paintSticky(QPainter &painter, int scrollY);
    void paintBand(QPainter &painter, const Band &band, int scrollX);
    void paintHeaderBand(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintLabels(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintNoteBand(QPainter &painter, const Band &band, const FileDiff &file, int viewW);
    void paintRow(QPainter &painter, const Band &band, const FileDiff &file, int viewW, int scrollX);
    void paintSingle(QPainter &painter, const Band &band, const FileDiff &file, const DiffRow &row, int viewW, int baseline, int scrollX);
    void paintPair(QPainter &painter, const Band &band, const FileDiff &file, const DiffRow &row, int viewW, int baseline, int scrollX);
    void paintSelection(QPainter &painter, const Band &band, int viewW);
    void paintOneSide(QPainter &painter, int cellX, int cellW, SideStyle style, const QString &text, int number,
                      const QList<WordSpan> &spans, bool note, int top, int height, int baseline, int scrollX);
    void paintFileHeader(QPainter &painter, const QRect &rect, const FileDiff &file, const QFont &font);
    bool pressIgnored(QMouseEvent *mouse) const;
    void chooseRow(const Band &band, int x);
    void setHover(int file, int row);
    void onPress(QMouseEvent *mouse);
    void onMove(QMouseEvent *mouse);
    void clearHover();
    const Band *bandAt(int y) const;
    bool inGutter(const FileDiff &file, int x, bool *oldSide) const;
    void emitVisibleFile();
    int fileAt(int y) const;
    bool headerStuck(int scrollY) const;
    int rowBaseline(const Band &band) const;

    DiffDoc m_doc;
    QString m_leftLabel;
    QString m_rightLabel;
    QString m_message;
    QList<Band> m_bands;
    QSet<QString> m_notes;
    QFont m_mono;
    int m_rowH = 20;
    int m_headerH = 34;
    int m_labelH = 24;
    int m_gutterW = 56;
    int m_docH = 0;
    int m_maxAdvance = 0;
    int m_selFile = -1;
    int m_selRow = -1;
    bool m_selOld = false;
    int m_hoverFile = -1;
    int m_hoverRow = -1;
    int m_lastFile = -1;
};
