// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QMouseEvent>
#include <QScrollBar>

namespace {

constexpr int kSplitGrab = 5;

} // namespace

bool DiffCanvas::layoutStale() const
{
    const int width = qMax(1, viewport()->width());
    return width != m_viewW || pairSplit(width) != m_splitW;
}

int DiffCanvas::sideOrigin(bool single, int x, bool *left) const
{
    const int viewW = viewport()->width();
    const int paneW = single ? viewW : pairSplit(viewW);
    *left = !single && x < paneW;
    return *left || single ? 0 : paneW;
}

int DiffCanvas::clampSplit(int viewW, int x) const
{
    const int minW = qMin(m_gutterW + 24, viewW / 2);
    return qBound(minW, x, qMax(minW, viewW - minW));
}

// Left pane width. The rest of the view is the right pane.
int DiffCanvas::pairSplit(int viewW) const
{
    if (viewW <= 0)
        return 0;
    return clampSplit(viewW, qRound(m_split * viewW));
}

bool DiffCanvas::onSplit(const Band *band, int x) const
{
    if (!band || (band->kind != Band::Labels && band->kind != Band::Row))
        return false;
    if (m_doc.files.at(band->file).singlePane())
        return false;
    return qAbs(x - pairSplit(viewport()->width())) <= kSplitGrab;
}

bool DiffCanvas::grabSplit(const Band *band, int x)
{
    if (!onSplit(band, x))
        return false;
    m_splitDx = pairSplit(qMax(1, viewport()->width())) - x;
    m_dragSplit = true;
    m_hoverFile = m_hoverRow = -1;
    viewport()->grabMouse();
    viewport()->setCursor(Qt::SplitHCursor);
    viewport()->update();
    return true;
}

bool DiffCanvas::dragSplit(int x, QMouseEvent *mouse)
{
    if (!m_dragSplit)
        return false;
    if (mouse->buttons().testFlag(Qt::LeftButton))
        moveSplit(x);
    return true;
}

void DiffCanvas::moveSplit(int x)
{
    const int viewW = qMax(1, viewport()->width());
    const int left = clampSplit(viewW, x + m_splitDx);
    m_split = double(left) / double(viewW);
    viewport()->update();
    if (left != m_splitW && !m_wrapTimer.isActive())
        m_wrapTimer.start();
}

void DiffCanvas::hoverMouse(QMouseEvent *mouse)
{
    const Band *band = bandAt(int(mouse->position().y()) + verticalScrollBar()->value());
    hoverRow(band);
    hoverCursor(band, int(mouse->position().x()));
}

void DiffCanvas::commitSplit()
{
    m_wrapTimer.stop();
    if (!m_doc.files.isEmpty() && layoutStale())
        rebuild();
}

bool DiffCanvas::releaseSplit(QMouseEvent *mouse)
{
    if (!m_dragSplit || mouse->button() != Qt::LeftButton)
        return false;
    m_dragSplit = false;
    viewport()->releaseMouse();
    commitSplit();
    hoverMouse(mouse);
    return true;
}

bool DiffCanvas::evenSplit(QMouseEvent *mouse)
{
    const int y = int(mouse->position().y()) + verticalScrollBar()->value();
    if (!onSplit(bandAt(y), int(mouse->position().x())))
        return false;
    m_split = 0.5;
    commitSplit();
    hoverMouse(mouse);
    return true;
}
