// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffSyntax.hpp"
#include "Syntax.hpp"

#include <oclero/qlementine/style/QlementineStyle.hpp>

#include <QApplication>
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
    setFocusPolicy(Qt::StrongFocus);
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
    int px = 13;
    if (const auto *style = qobject_cast<const oclero::qlementine::QlementineStyle *>(qApp->style()))
        px = style->theme().fontMonospace.pixelSize();
    if (px > 0)
        m_mono.setPixelSize(px);
    const QFontMetrics metrics(m_mono);
    m_rowH = metrics.height() + 4;
    m_gutterW = metrics.horizontalAdvance(QStringLiteral("00000")) + 22;
}

void DiffCanvas::setMessage(const QString &text)
{
    m_wrapTimer.stop();
    m_message = text;
    m_doc = {};
    m_open.clear();
    m_wrapW.clear();
    m_viewW = -1;
    m_bands.clear();
    m_docH = 0;
    m_selFile = m_selRow = m_hoverFile = m_hoverRow = m_lastFile = -1;
    clearMark();
    updateScroll();
    viewport()->update();
}

void DiffCanvas::setDoc(const DiffDoc &doc, const QString &leftLabel, const QString &rightLabel)
{
    m_wrapTimer.stop();
    m_message.clear();
    m_doc = doc;
    m_open = QVector<QSet<int>>(m_doc.files.size());
    m_wrapW = QVector<int>(m_doc.files.size(), -1);
    m_viewW = -1;
    highlightDoc(&m_doc);
    m_leftLabel = leftLabel;
    m_rightLabel = rightLabel;
    m_selFile = m_selRow = m_hoverFile = m_hoverRow = m_lastFile = -1;
    clearMark();
    rebuild();
}

void DiffCanvas::wrapRow(DiffRow *row, bool single)
{
    const int width = m_viewW > 0 ? m_viewW : 1;
    if (single) {
        row->leftPiece.clear();
        row->rightPiece = sidePieces(row->rightText, row->rightSpans, row->rightSyn, m_mono, paneText(width, m_gutterW));
        return;
    }
    const int left = pairSplit(width);
    row->leftPiece = sidePieces(row->leftText, row->leftSpans, row->leftSyn, m_mono, paneText(left, m_gutterW));
    row->rightPiece = sidePieces(row->rightText, row->rightSpans, row->rightSyn, m_mono, paneText(width - left, m_gutterW));
}

int DiffCanvas::rowHeight(const DiffRow &row, bool single) const
{
    const int lines = single ? visualLines(row.rightPiece) : qMax(visualLines(row.leftPiece), visualLines(row.rightPiece));
    return lines * m_rowH;
}

void DiffCanvas::addFileBands(int fileIndex, int *y)
{
    const FileDiff &file = m_doc.files.at(fileIndex);
    m_bands.push_back(Band{Band::Header, fileIndex, -1, *y, m_headerH});
    *y += m_headerH;
    m_bands.push_back(Band{Band::Labels, fileIndex, -1, *y, m_labelH});
    *y += m_labelH;
    if (file.binary || file.rows.isEmpty()) {
        m_bands.push_back(Band{Band::Note, fileIndex, -1, *y, m_rowH});
        *y += m_rowH;
    } else {
        addRowBands(fileIndex, y);
    }
    *y += 16;
}

void DiffCanvas::rebuild()
{
    m_wrapTimer.stop();
    ensureFont();
    prepareWidth();
    m_bands.clear();
    int y = 8;
    for (int fileIndex = 0; fileIndex < m_doc.files.size(); ++fileIndex)
        addFileBands(fileIndex, &y);
    m_docH = y + 12;
    publishLayout();
}
