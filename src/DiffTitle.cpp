// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QMouseEvent>
#include <QScrollBar>

bool DiffCanvas::stickyZone(int viewY, int scrollY) const
{
    return headerStuck(scrollY) && viewY >= 0 && viewY < m_headerH;
}

int DiffCanvas::stuckTitle(int scrollY, QRect *check) const
{
    const int file = fileAt(scrollY);
    if (file < 0)
        return -1;
    *check = titleCheck(QRect(0, 0, viewport()->width(), m_headerH));
    return file;
}

int DiffCanvas::bandTitle(int viewY, int scrollY, QRect *check) const
{
    const Band *band = bandAt(viewY + scrollY);
    if (!band || band->kind != Band::Header)
        return -1;
    *check = titleCheck(QRect(0, band->y - scrollY, viewport()->width(), band->h));
    return band->file;
}

int DiffCanvas::titleFile(int viewY, QRect *check) const
{
    const int scrollY = verticalScrollBar()->value();
    if (stickyZone(viewY, scrollY))
        return stuckTitle(scrollY, check);
    return bandTitle(viewY, scrollY, check);
}

bool DiffCanvas::titlePress(int x, int viewY)
{
    QRect check;
    const int file = titleFile(viewY, &check);
    if (file < 0 || !check.contains(x, viewY))
        return false;
    emit reviewRequested(file);
    return true;
}

bool DiffCanvas::hoverTitle(int x, int viewY)
{
    QRect check;
    const int file = titleFile(viewY, &check);
    const int hot = file >= 0 && check.contains(x, viewY) ? file : -1;
    if (hot == m_checkFile)
        return hot >= 0;
    m_checkFile = hot;
    setToolTip(hot >= 0 ? QStringLiteral("Mark as reviewed") : QString());
    if (hot >= 0) {
        m_hoverFile = m_hoverRow = -1;
        viewport()->setCursor(Qt::PointingHandCursor);
    }
    viewport()->update();
    return hot >= 0;
}

void DiffCanvas::takePress(QMouseEvent *mouse)
{
    const int x = int(mouse->position().x());
    const int viewY = int(mouse->position().y());
    if (titlePress(x, viewY))
        return;
    if (headerPress(mouse))
        return;
    pressAt(mouse, x, viewY + verticalScrollBar()->value());
}
