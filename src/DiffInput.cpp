// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

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
    bool left = false;
    const int origin = sideOrigin(file.singlePane(), x, &left);
    if (oldSide)
        *oldSide = left;
    return x - origin >= 0 && x - origin < m_gutterW;
}

bool DiffCanvas::pressIgnored(QMouseEvent *mouse) const
{
    return mouse->button() != Qt::LeftButton || !m_message.isEmpty();
}

bool DiffCanvas::headerPress(QMouseEvent *mouse) const
{
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

void DiffCanvas::pressRow(const Band &band, int x, int y, QMouseEvent *mouse)
{
    if (inGutter(m_doc.files.at(band.file), x, nullptr)) {
        chooseRow(band, x);
        clearMark();
        return;
    }
    if (!mouse->modifiers().testFlag(Qt::ShiftModifier) && takeLine(x, y))
        return;
    beginText(x, y, mouse->modifiers().testFlag(Qt::ShiftModifier));
}

void DiffCanvas::pressAt(QMouseEvent *mouse, int x, int y)
{
    if (jumpMod(mouse->modifiers())) {
        chooseDef(x, y);
        return;
    }
    const Band *band = bandAt(y);
    if (!band || openFold(band, x) || grabSplit(band, x) || band->kind != Band::Row)
        return;
    pressRow(*band, x, y, mouse);
}

void DiffCanvas::onPress(QMouseEvent *mouse)
{
    if (pressIgnored(mouse))
        return;
    setFocus();
    takePress(mouse);
}

void DiffCanvas::clearHover()
{
    m_hoverFile = m_hoverRow = -1;
    m_checkFile = -1;
    setToolTip(QString());
    if (!m_dragSplit)
        viewport()->unsetCursor();
    viewport()->update();
    dropDef();
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
    Qt::CursorShape shape = Qt::ArrowCursor;
    if (onSplit(band, x))
        shape = Qt::SplitHCursor;
    else if (gutter || fold)
        shape = Qt::PointingHandCursor;
    else if (file >= 0)
        shape = Qt::IBeamCursor;
    viewport()->setCursor(shape);
}

void DiffCanvas::trackHover(QMouseEvent *mouse, int x, int y)
{
    if (hoverTitle(x, int(mouse->position().y()))) {
        stopDef();
        return;
    }
    const Band *band = bandAt(y);
    hoverRow(band);
    hoverCursor(band, x);
    hoverDef(x, y);
    followHand();
}

void DiffCanvas::onMove(QMouseEvent *mouse)
{
    const int x = int(mouse->position().x());
    const int y = int(mouse->position().y()) + verticalScrollBar()->value();
    if (dragSplit(x, mouse))
        return;
    if (m_dragText && mouse->buttons().testFlag(Qt::LeftButton)) {
        moveDrag(x, y);
        return;
    }
    trackHover(mouse, x, y);
}

bool DiffCanvas::mouseEvent(QEvent *event)
{
    auto *mouse = static_cast<QMouseEvent *>(event);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        onPress(mouse);
        return true;
    case QEvent::MouseButtonRelease:
        if (!releaseSplit(mouse))
            endText(mouse);
        return true;
    case QEvent::MouseButtonDblClick:
        if (!jumpMod(mouse->modifiers()))
            pickWord(mouse);
        return true;
    case QEvent::MouseMove:
        onMove(mouse);
        return true;
    default:
        return false;
    }
}

bool DiffCanvas::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        paintContents();
        return true;
    }
    if (event->type() == QEvent::Leave) {
        clearHover();
        return true;
    }
    return mouseEvent(event) || QAbstractScrollArea::viewportEvent(event);
}
