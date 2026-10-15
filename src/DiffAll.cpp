// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QScrollBar>

int DiffCanvas::paneFile() const
{
    if (m_anchor.file >= 0)
        return m_anchor.file;
    if (m_selFile >= 0 && rowShown(m_selFile, m_selRow))
        return m_selFile;
    return fileAt(verticalScrollBar()->value() + textTop());
}

bool DiffCanvas::paneOld(int file) const
{
    const FileDiff &diff = m_doc.files.at(file);
    if (diff.removed)
        return true;
    if (diff.added || diff.singlePane())
        return false;
    if (m_anchor.file == file)
        return m_anchor.old;
    if (m_selFile == file)
        return m_selOld;
    return false;
}

namespace {

void noteSide(bool old, int row, const DiffRow &line, int *first, int *last, int *end)
{
    if ((old ? line.leftNum : line.rightNum) <= 0)
        return;
    if (*first < 0)
        *first = row;
    *last = row;
    if (end)
        *end = (old ? line.leftText : line.rightText).size();
}

} // namespace

bool DiffCanvas::sideEnd(int file, bool old, int *first, int *last, int *end) const
{
    *first = -1;
    *last = -1;
    if (end)
        *end = 0;
    if (file < 0 || file >= m_doc.files.size())
        return false;
    const FileDiff &diff = m_doc.files.at(file);
    for (int row = 0; row < diff.rows.size(); ++row)
        noteSide(old, row, diff.rows.at(row), first, last, end);
    return *first >= 0;
}

bool DiffCanvas::fullMark(const TextMark &from, const TextMark &to) const
{
    int first = -1;
    int last = -1;
    int end = 0;
    if (!sideEnd(from.file, from.old, &first, &last, &end))
        return false;
    return from.row == first && from.pos == 0 && to.row == last && to.pos == end;
}

void DiffCanvas::dropDrag()
{
    if (!m_dragText)
        return;
    m_dragTimer.stop();
    m_dragText = false;
    viewport()->releaseMouse();
}

void DiffCanvas::showMark(int file, bool old, int first, int last, int end)
{
    TextMark mark;
    mark.file = file;
    mark.row = first;
    mark.old = old;
    mark.code = true;
    m_anchor = mark;
    mark.row = last;
    mark.pos = end;
    m_caret = mark;
    m_selFile = file;
    m_selRow = first;
    m_selOld = old;
    m_goal = -1;
    m_wordMs = 0;
    viewport()->update();
}

void DiffCanvas::selectAll()
{
    const int file = paneFile();
    if (file < 0 || file >= m_doc.files.size())
        return;
    const bool old = paneOld(file);
    int first = 0;
    int last = 0;
    int end = 0;
    if (!sideEnd(file, old, &first, &last, &end))
        return;
    dropDrag();
    showMark(file, old, first, last, end);
}
