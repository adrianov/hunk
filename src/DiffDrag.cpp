// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QCursor>
#include <QGuiApplication>
#include <QScrollBar>

namespace {

// Signed pixels past the text area. The sticky header counts as the top edge.
int pastEdge(int viewY, int top, int bottom)
{
    if (viewY < top)
        return viewY - top;
    if (viewY >= bottom)
        return viewY - (bottom - 1);
    return 0;
}

constexpr int kScrollSpan = 12;

// One tick of drag-scroll. At the edge this is one line; further out it speeds up.
int scrollStep(int outside, int rowH)
{
    const int distance = qAbs(outside);
    if (!distance)
        return 0;
    const int speed = qBound(qMax(1, rowH), distance, qMax(1, rowH) * kScrollSpan);
    return outside < 0 ? -speed : speed;
}

bool rowsBack(int row, int caret, bool upward)
{
    if (row == caret)
        return false;
    return upward ? row > caret : row < caret;
}

} // namespace

void DiffCanvas::armDrag()
{
    if (m_dragTimer.interval() > 0)
        return;
    m_dragTimer.setInterval(50);
    connect(&m_dragTimer, &QTimer::timeout, this, &DiffCanvas::scrollDrag);
}

bool DiffCanvas::dragHeld()
{
    if (m_dragText && QGuiApplication::mouseButtons().testFlag(Qt::LeftButton))
        return true;
    m_dragTimer.stop();
    if (!m_dragText)
        return false;
    m_dragText = false;
    viewport()->releaseMouse();
    return false;
}

int DiffCanvas::textTop() const
{
    return headerStuck(verticalScrollBar()->value()) ? m_headerH : 0;
}

int DiffCanvas::outsideAt(int viewY) const
{
    return pastEdge(viewY, textTop(), qMax(1, viewport()->height()));
}

bool DiffCanvas::rollDrag(int outside)
{
    QScrollBar *bar = verticalScrollBar();
    const int next = qBound(bar->minimum(), bar->value() + scrollStep(outside, m_rowH), bar->maximum());
    if (next == bar->value())
        return false;
    bar->setValue(next);
    return true;
}

void DiffCanvas::paceDrag(int outside)
{
    if (rollDrag(outside)) {
        if (!m_dragTimer.isActive())
            m_dragTimer.start();
        return;
    }
    m_dragTimer.stop();
}

int DiffCanvas::edgeDoc(int viewY) const
{
    return qBound(textTop(), viewY, qMax(1, viewport()->height()) - 1) + verticalScrollBar()->value();
}

void DiffCanvas::moveDrag(int x, int y)
{
    if (!outsideAt(y - verticalScrollBar()->value())) {
        m_dragTimer.stop();
        dragText(x, y);
        return;
    }
    if (m_dragTimer.isActive())
        return;
    armDrag();
    scrollDrag();
}

void DiffCanvas::scrollDrag()
{
    if (!dragHeld())
        return;
    const QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
    trackDrag(pos.x(), pos.y());
}

void DiffCanvas::trackDrag(int x, int viewY)
{
    const int outside = outsideAt(viewY);
    const int y = edgeDoc(viewY);
    if (!outside) {
        m_dragTimer.stop();
        dragText(x, y);
        return;
    }
    paceDrag(outside);
    dragEdge(x, y, outside < 0);
}

int DiffCanvas::paneX(int x) const
{
    const FileDiff &file = m_doc.files.at(m_anchor.file);
    const int viewW = qMax(1, viewport()->width());
    const int paneW = file.singlePane() ? viewW : pairSplit(viewW);
    const bool left = file.singlePane() || m_anchor.old;
    const int origin = left ? 0 : paneW;
    const int width = left ? paneW : viewW - paneW;
    const int code = origin + m_gutterW + 8;
    return qBound(code, x, qMax(code, origin + width - 8));
}

void DiffCanvas::dragEdge(int x, int y, bool upward)
{
    if (m_anchor.file < 0 || m_anchor.file >= m_doc.files.size())
        return;
    const int atX = paneX(x);
    const int step = upward ? m_rowH : -m_rowH;
    for (int n = 0; n <= kScrollSpan; ++n) {
        const int at = y + step * n;
        const TextMark hit = textAt(atX, at);
        if (!hit.code || hit.file != m_anchor.file || hit.old != m_anchor.old)
            continue;
        if (rowsBack(hit.row, m_caret.row, upward))
            return;
        dragText(atX, at);
        return;
    }
}
