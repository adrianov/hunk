// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffColors.hpp"
#include "DiffFold.hpp"
#include "ReviewNote.hpp"

#include <QPainter>

namespace {

QString foldText(int count)
{
    if (count == 1)
        return QStringLiteral("Show 1 unchanged line");
    return QStringLiteral("Show %1 unchanged lines").arg(count);
}

void paintFoldRule(QPainter &painter, const QRect &rect)
{
    painter.setPen(kLine);
    painter.drawLine(rect.topLeft(), rect.topRight());
    painter.drawLine(rect.bottomLeft(), rect.bottomRight());
}

void paintFoldEdges(QPainter &painter, const QRect &rect, int count)
{
    paintFoldRule(painter, rect);
    painter.setPen(kFile);
    if (count <= kFoldStep)
        return;
    const QString down = QStringLiteral("↓ %1").arg(kFoldStep);
    const QString up = QStringLiteral("%1 ↑").arg(kFoldStep);
    painter.drawText(rect.adjusted(8, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, down);
    painter.drawText(rect.adjusted(0, 0, -8, 0), Qt::AlignVCenter | Qt::AlignRight, up);
}

bool notedRow(const FileDiff &file, const DiffRow &row, const QSet<QString> &notes)
{
    return (row.leftNum > 0 && notes.contains(noteKey(file.path(), true, row.leftNum)))
        || (row.rightNum > 0 && notes.contains(noteKey(file.path(), false, row.rightNum)));
}

bool searchedRow(const DiffRow &row, const QString &query)
{
    if (query.isEmpty())
        return false;
    return (row.leftNum > 0 && row.leftText.contains(query, Qt::CaseInsensitive))
        || (row.rightNum > 0 && row.rightText.contains(query, Qt::CaseInsensitive));
}

} // namespace

QSet<int> DiffCanvas::pinnedRows(int fileIndex) const
{
    QSet<int> open = m_open.value(fileIndex);
    QSet<int> hits;
    const FileDiff &file = m_doc.files.at(fileIndex);
    for (int index = 0; index < file.rows.size(); ++index) {
        const DiffRow &row = file.rows.at(index);
        if (notedRow(file, row, m_notes))
            open.insert(index);
        if (searchedRow(row, m_search))
            hits.insert(index);
    }
    return unfoldHits(file.rows, open, hits);
}

void DiffCanvas::showHidden(int file, int first, int last)
{
    if (file < 0 || file >= m_open.size() || last < first)
        return;
    for (int row = first; row <= last; ++row)
        m_open[file].insert(row);
    relayoutFile(file);
    publishLayout();
}

void DiffCanvas::revealRow(int file, int row)
{
    for (const Band &band : m_bands) {
        if (band.file == file && band.kind == Band::Fold && row >= band.row && row <= band.end) {
            const int first = band.row;
            const int last = band.end;
            showHidden(file, first, last);
            return;
        }
    }
}

void DiffCanvas::addFoldBand(int fileIndex, const FoldSpan &span, int *y)
{
    m_bands.push_back(Band{Band::Fold, fileIndex, span.first, *y, m_rowH});
    m_bands.last().end = span.last;
    *y += m_rowH;
}

void DiffCanvas::addCodeBand(int fileIndex, int rowIndex, bool single, int *y)
{
    ensureRow(fileIndex, rowIndex, single);
    const int height = rowHeight(m_doc.files.at(fileIndex).rows.at(rowIndex), single);
    m_bands.push_back(Band{Band::Row, fileIndex, rowIndex, *y, height});
    *y += height;
}

void DiffCanvas::addRowBands(int fileIndex, int *y)
{
    const FileDiff &file = m_doc.files.at(fileIndex);
    const bool single = file.singlePane();
    const QList<FoldSpan> folds = foldSpans(file.rows, pinnedRows(fileIndex));
    int foldAt = 0;
    for (int rowIndex = 0; rowIndex < file.rows.size();) {
        if (foldAt < folds.size() && folds.at(foldAt).first == rowIndex) {
            addFoldBand(fileIndex, folds.at(foldAt), y);
            rowIndex = folds.at(foldAt).last + 1;
            ++foldAt;
            continue;
        }
        addCodeBand(fileIndex, rowIndex, single, y);
        ++rowIndex;
    }
}

bool DiffCanvas::openFold(const Band *band, int x)
{
    if (!band || band->kind != Band::Fold)
        return false;
    int first = band->row;
    int last = band->end;
    if (x < kFoldEdge)
        last = qMin(last, first + kFoldStep - 1);
    else if (x >= viewport()->width() - kFoldEdge)
        first = qMax(first, last - kFoldStep + 1);
    showHidden(band->file, first, last);
    return true;
}

void DiffCanvas::paintFold(QPainter &painter, const Band &band, int viewW)
{
    const QRect rect(0, band.y, viewW, band.h);
    const int count = band.end - band.row + 1;
    painter.fillRect(rect, kHeaderBg);
    painter.setFont(font());
    painter.setPen(kFile);
    painter.drawText(rect.adjusted(kFoldEdge, 0, -kFoldEdge, 0), Qt::AlignCenter, foldText(count));
    paintFoldEdges(painter, rect, count);
}
