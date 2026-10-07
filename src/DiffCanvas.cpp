#include "DiffCanvas.hpp"

#include "DiffSyntax.hpp"
#include "Syntax.hpp"

#include <QFontDatabase>
#include <QFontMetrics>
#include <QScrollBar>

namespace {

int paneText(int pane, int gutter)
{
    const int width = pane - gutter - 16;
    return width > 0 ? width : 1;
}

int visualLines(const QList<Piece> &pieces)
{
    int lines = 0;
    for (const Piece &piece : pieces)
        lines += piece.newLine ? 1 : 0;
    return lines > 0 ? lines : 1;
}

} // namespace

DiffCanvas::DiffCanvas(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    ensureFont();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_wrapTimer.setSingleShot(true);
    m_wrapTimer.setInterval(100);
    connect(&m_wrapTimer, &QTimer::timeout, this, &DiffCanvas::rebuild);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        viewport()->update();
        emitVisibleFile();
    });
}

void DiffCanvas::ensureFont()
{
    m_mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_mono.setPointSize(12);
    const QFontMetrics metrics(m_mono);
    m_rowH = metrics.height() + 6;
    m_gutterW = qMax(56, metrics.horizontalAdvance(QStringLiteral("00000")) + 18);
}

void DiffCanvas::setMessage(const QString &text)
{
    m_wrapTimer.stop();
    m_message = text;
    m_doc = {};
    m_bands.clear();
    m_docH = 0;
    m_selFile = m_selRow = m_hoverFile = m_hoverRow = m_lastFile = -1;
    updateScroll();
    viewport()->update();
}

void DiffCanvas::setDoc(const DiffDoc &doc, const QString &leftLabel, const QString &rightLabel)
{
    m_wrapTimer.stop();
    m_message.clear();
    m_doc = doc;
    highlightDoc(&m_doc);
    m_leftLabel = leftLabel;
    m_rightLabel = rightLabel;
    m_selFile = m_selRow = m_hoverFile = m_hoverRow = m_lastFile = -1;
    rebuild();
}

void DiffCanvas::setNoteKeys(const QSet<QString> &keys)
{
    m_notes = keys;
    viewport()->update();
}

void DiffCanvas::wrapRow(DiffRow *row, bool single)
{
    int width = viewport()->width();
    if (width <= 0)
        width = 1;
    if (single) {
        row->leftPiece.clear();
        row->rightPiece = sidePieces(row->rightText, row->rightSpans, row->rightSyn, m_mono, paneText(width, m_gutterW));
        return;
    }
    row->leftPiece = sidePieces(row->leftText, row->leftSpans, row->leftSyn, m_mono, paneText(width / 2, m_gutterW));
    row->rightPiece = sidePieces(row->rightText, row->rightSpans, row->rightSyn, m_mono, paneText(width - width / 2, m_gutterW));
}

int DiffCanvas::rowHeight(const DiffRow &row, bool single) const
{
    const int lines = single ? visualLines(row.rightPiece) : qMax(visualLines(row.leftPiece), visualLines(row.rightPiece));
    return lines * m_rowH;
}

void DiffCanvas::addFileBands(int fileIndex, int *y)
{
    FileDiff &file = m_doc.files[fileIndex];
    m_bands.push_back(Band{Band::Header, fileIndex, -1, *y, m_headerH});
    *y += m_headerH;
    m_bands.push_back(Band{Band::Labels, fileIndex, -1, *y, m_labelH});
    *y += m_labelH;
    if (file.binary || file.rows.isEmpty()) {
        m_bands.push_back(Band{Band::Note, fileIndex, -1, *y, m_rowH});
        *y += m_rowH;
    } else {
        const bool single = file.singlePane();
        for (int rowIndex = 0; rowIndex < file.rows.size(); ++rowIndex) {
            DiffRow &row = file.rows[rowIndex];
            wrapRow(&row, single);
            const int height = rowHeight(row, single);
            m_bands.push_back(Band{Band::Row, fileIndex, rowIndex, *y, height});
            *y += height;
        }
    }
    *y += 16;
}

void DiffCanvas::rebuild()
{
    ensureFont();
    m_bands.clear();
    int y = 8;
    for (int fileIndex = 0; fileIndex < m_doc.files.size(); ++fileIndex)
        addFileBands(fileIndex, &y);
    m_viewW = viewport()->width();
    m_docH = y + 12;
    updateScroll();
    viewport()->update();
    emitVisibleFile();
}
