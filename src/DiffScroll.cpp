#include "DiffCanvas.hpp"

#include <QResizeEvent>
#include <QScrollBar>

void DiffCanvas::showFile(int file)
{
    for (const Band &band : m_bands) {
        if (band.file == file && band.kind == Band::Header) {
            verticalScrollBar()->setValue(band.y);
            return;
        }
    }
}

void DiffCanvas::showRow(int file, int row, bool oldSide)
{
    m_selFile = file;
    m_selRow = row;
    m_selOld = oldSide;
    for (const Band &band : m_bands) {
        if (band.file == file && band.kind == Band::Row && band.row == row) {
            verticalScrollBar()->setValue(qMax(0, band.y - m_headerH - 8));
            break;
        }
    }
    viewport()->update();
}

int DiffCanvas::scrollTop() const
{
    return verticalScrollBar()->value();
}

void DiffCanvas::setScrollTop(int y)
{
    verticalScrollBar()->setValue(y);
}

void DiffCanvas::updateVertical(int viewH)
{
    verticalScrollBar()->setPageStep(viewH);
    verticalScrollBar()->setSingleStep(m_rowH);
    verticalScrollBar()->setRange(0, qMax(0, m_docH - viewH));
}

void DiffCanvas::updateScroll()
{
    updateVertical(qMax(1, viewport()->height()));
}

void DiffCanvas::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScroll();
    viewport()->update();
    if (viewport()->width() != m_viewW && !m_doc.files.isEmpty())
        m_wrapTimer.start();
}

int DiffCanvas::fileAt(int y) const
{
    int file = -1;
    for (const Band &band : m_bands) {
        if (band.y <= y)
            file = band.file;
    }
    return file;
}

bool DiffCanvas::headerStuck(int scrollY) const
{
    int headerY = -1;
    for (const Band &band : m_bands) {
        if (band.kind == Band::Header && band.y <= scrollY)
            headerY = band.y;
    }
    return headerY >= 0 && headerY < scrollY;
}

void DiffCanvas::emitVisibleFile()
{
    if (m_bands.isEmpty())
        return;
    const int scrollY = verticalScrollBar()->value();
    const int y = scrollY + (headerStuck(scrollY) ? m_headerH : 0) + 2;
    const int file = fileAt(y);
    if (file < 0 || file == m_lastFile)
        return;
    m_lastFile = file;
    emit fileScrolled(file);
}
