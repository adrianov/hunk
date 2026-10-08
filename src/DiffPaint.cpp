#include "DiffCanvas.hpp"

#include "DiffColors.hpp"
#include "ReviewNote.hpp"

#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>

namespace {

SideStyle leftStyle(RowKind kind)
{
    if (kind == RowKind::Add)
        return SideStyle::Empty;
    if (kind == RowKind::Del || kind == RowKind::Mod)
        return SideStyle::Del;
    return SideStyle::Plain;
}

SideStyle rightStyle(RowKind kind)
{
    if (kind == RowKind::Del)
        return SideStyle::Empty;
    if (kind == RowKind::Add || kind == RowKind::Mod)
        return SideStyle::Add;
    return SideStyle::Plain;
}

void paintMessage(QPainter &painter, const QRect &rect, const QFont &font, const QString &message)
{
    painter.setPen(kMuted);
    painter.setFont(font);
    painter.drawText(rect.adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap, message);
}

} // namespace

void DiffCanvas::paintSingle(QPainter &painter, const Band &band, const FileDiff &file, const DiffRow &row, int viewW)
{
    const bool note = row.rightNum > 0 && m_notes.contains(noteKey(file.path(), false, row.rightNum));
    paintOneSide(painter, 0, viewW, SideStyle::Add, row.rightText, row.rightNum, row.rightPiece, note, band.y, band.h);
}

void DiffCanvas::paintPair(QPainter &painter, const Band &band, const FileDiff &file, const DiffRow &row, int viewW)
{
    const int paneW = viewW / 2;
    const bool leftNote = row.leftNum > 0 && m_notes.contains(noteKey(file.path(), true, row.leftNum));
    const bool rightNote = row.rightNum > 0 && m_notes.contains(noteKey(file.path(), false, row.rightNum));
    paintOneSide(painter, 0, paneW, leftStyle(row.kind), row.leftText, row.leftNum, row.leftPiece, leftNote, band.y,
                 band.h);
    paintOneSide(painter, paneW, viewW - paneW, rightStyle(row.kind), row.rightText, row.rightNum, row.rightPiece,
                 rightNote, band.y, band.h);
    painter.setPen(kLine);
    painter.drawLine(paneW, band.y, paneW, band.y + band.h);
}

void DiffCanvas::paintSelection(QPainter &painter, const Band &band, int viewW)
{
    const bool selected = band.file == m_selFile && band.row == m_selRow;
    const bool hovered = band.file == m_hoverFile && band.row == m_hoverRow;
    if (!selected && !hovered)
        return;
    QColor hover = kText;
    hover.setAlpha(48);
    painter.setPen(selected ? kAccent : hover);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, band.y, viewW - 1, band.h - 1);
}

void DiffCanvas::paintRow(QPainter &painter, const Band &band, const FileDiff &file, int viewW)
{
    const DiffRow &row = file.rows.at(band.row);
    if (file.singlePane())
        paintSingle(painter, band, file, row, viewW);
    else
        paintPair(painter, band, file, row, viewW);
    paintSelection(painter, band, viewW);
}

void DiffCanvas::paintBand(QPainter &painter, const Band &band)
{
    const int viewW = viewport()->width();
    const FileDiff &file = m_doc.files.at(band.file);
    if (band.kind == Band::Header)
        paintHeaderBand(painter, band, file, viewW);
    else if (band.kind == Band::Labels)
        paintLabels(painter, band, file, viewW);
    else if (band.kind == Band::Note)
        paintNoteBand(painter, band, file, viewW);
    else if (band.kind == Band::Fold)
        paintFold(painter, band, viewW);
    else
        paintRow(painter, band, file, viewW);
}

void DiffCanvas::paintVisible(QPainter &painter, int scrollY)
{
    for (const Band &band : m_bands) {
        if (band.y + band.h < scrollY || band.y > scrollY + viewport()->height())
            continue;
        paintBand(painter, band);
    }
}

void DiffCanvas::paintSticky(QPainter &painter, int scrollY)
{
    if (!headerStuck(scrollY))
        return;
    const int file = fileAt(scrollY);
    if (file < 0)
        return;
    QFont headerFont = font();
    headerFont.setBold(true);
    paintFileHeader(painter, QRect(0, 0, viewport()->width(), m_headerH), m_doc.files.at(file), headerFont);
}

void DiffCanvas::paintDoc(QPainter &painter)
{
    const int scrollY = verticalScrollBar()->value();
    painter.save();
    painter.translate(0, -scrollY);
    painter.setClipRect(0, scrollY, viewport()->width(), viewport()->height());
    paintVisible(painter, scrollY);
    painter.restore();
    paintSticky(painter, scrollY);
}

void DiffCanvas::paintContents()
{
    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), kBg);
    if (!m_message.isEmpty()) {
        paintMessage(painter, viewport()->rect(), font(), m_message);
        return;
    }
    paintDoc(painter);
}
