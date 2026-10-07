#include "DiffCanvas.hpp"

#include <QMouseEvent>
#include <QScrollBar>

const DiffCanvas::Band *DiffCanvas::bandAt(int y) const
{
    for (const Band &band : m_bands) {
        if (y >= band.y && y < band.y + band.h)
            return &band;
    }
    return nullptr;
}

bool DiffCanvas::inGutter(const FileDiff &file, int x, bool *oldSide) const
{
    const int viewW = viewport()->width();
    const int paneW = file.singlePane() ? viewW : viewW / 2;
    const bool left = !file.singlePane() && x < paneW;
    if (oldSide)
        *oldSide = left;
    const int origin = left ? 0 : (file.singlePane() ? 0 : paneW);
    return x - origin >= 0 && x - origin < m_gutterW;
}

bool DiffCanvas::pressIgnored(QMouseEvent *mouse) const
{
    if (mouse->button() != Qt::LeftButton || !m_message.isEmpty())
        return true;
    return headerStuck(verticalScrollBar()->value()) && mouse->position().y() < m_headerH;
}

void DiffCanvas::chooseRow(const Band &band, int x)
{
    bool oldSide = false;
    const bool gutter = inGutter(m_doc.files.at(band.file), x, &oldSide);
    m_selFile = band.file;
    m_selRow = band.row;
    m_selOld = oldSide;
    viewport()->update();
    if (gutter)
        emit commentRequested(band.file, band.row, oldSide);
}

void DiffCanvas::pressBand(const Band *band, int x)
{
    if (!band || openFold(band, x) || band->kind != Band::Row)
        return;
    chooseRow(*band, x);
}

void DiffCanvas::onPress(QMouseEvent *mouse)
{
    if (pressIgnored(mouse))
        return;
    const int y = int(mouse->position().y()) + verticalScrollBar()->value();
    pressBand(bandAt(y), int(mouse->position().x()));
}

void DiffCanvas::clearHover()
{
    m_hoverFile = m_hoverRow = -1;
    viewport()->unsetCursor();
    viewport()->update();
}

void DiffCanvas::setHover(int file, int row)
{
    if (file == m_hoverFile && row == m_hoverRow)
        return;
    m_hoverFile = file;
    m_hoverRow = row;
    viewport()->update();
}

void DiffCanvas::hoverRow(const Band *band)
{
    const int file = band && band->kind == Band::Row ? band->file : -1;
    const int row = band && band->kind == Band::Row ? band->row : -1;
    setHover(file, row);
}

void DiffCanvas::hoverCursor(const Band *band, int x)
{
    const int file = band && band->kind == Band::Row ? band->file : -1;
    const bool gutter = file >= 0 && inGutter(m_doc.files.at(file), x, nullptr);
    const bool fold = band && band->kind == Band::Fold;
    viewport()->setCursor(gutter || fold ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void DiffCanvas::onMove(QMouseEvent *mouse)
{
    const int y = int(mouse->position().y()) + verticalScrollBar()->value();
    const Band *band = bandAt(y);
    hoverRow(band);
    hoverCursor(band, int(mouse->position().x()));
}

bool DiffCanvas::viewportEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Paint:
        paintContents();
        return true;
    case QEvent::MouseButtonPress:
        onPress(static_cast<QMouseEvent *>(event));
        return true;
    case QEvent::MouseMove:
        onMove(static_cast<QMouseEvent *>(event));
        return true;
    case QEvent::Leave:
        clearHover();
        return true;
    default:
        return QAbstractScrollArea::viewportEvent(event);
    }
}
