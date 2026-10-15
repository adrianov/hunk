// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QKeyEvent>
#include <QScrollBar>

namespace {

enum MoveKind {
    MoveNextChar = 1,
    MovePrevChar = -1,
    MoveNextLine = 2,
    MovePrevLine = -2,
    MoveNextWord = 3,
    MovePrevWord = -3,
    MoveLineEnd = 4,
    MoveLineStart = -4,
    MoveDocEnd = 5,
    MoveDocStart = -5,
};

struct MoveBind {
    QKeySequence::StandardKey key;
    int kind;
    bool extend;
};

const MoveBind kMoves[] = {
    {QKeySequence::SelectNextChar, MoveNextChar, true},
    {QKeySequence::SelectPreviousChar, MovePrevChar, true},
    {QKeySequence::SelectNextLine, MoveNextLine, true},
    {QKeySequence::SelectPreviousLine, MovePrevLine, true},
    {QKeySequence::SelectNextWord, MoveNextWord, true},
    {QKeySequence::SelectPreviousWord, MovePrevWord, true},
    {QKeySequence::SelectEndOfLine, MoveLineEnd, true},
    {QKeySequence::SelectStartOfLine, MoveLineStart, true},
    {QKeySequence::SelectEndOfDocument, MoveDocEnd, true},
    {QKeySequence::SelectStartOfDocument, MoveDocStart, true},
    {QKeySequence::MoveToNextChar, MoveNextChar, false},
    {QKeySequence::MoveToPreviousChar, MovePrevChar, false},
    {QKeySequence::MoveToNextLine, MoveNextLine, false},
    {QKeySequence::MoveToPreviousLine, MovePrevLine, false},
    {QKeySequence::MoveToNextWord, MoveNextWord, false},
    {QKeySequence::MoveToPreviousWord, MovePrevWord, false},
    {QKeySequence::MoveToEndOfLine, MoveLineEnd, false},
    {QKeySequence::MoveToStartOfLine, MoveLineStart, false},
    {QKeySequence::MoveToEndOfDocument, MoveDocEnd, false},
    {QKeySequence::MoveToStartOfDocument, MoveDocStart, false},
};

int moveKind(const QKeyEvent *key, bool *extend)
{
    for (const MoveBind &item : kMoves) {
        if (!key->matches(item.key))
            continue;
        *extend = item.extend;
        return item.kind;
    }
    return 0;
}

} // namespace

bool DiffCanvas::movePress(const QKeyEvent *key)
{
    if (!paneFocus())
        return false;
    bool extend = false;
    const int kind = moveKind(key, &extend);
    if (!kind || !placeCaret())
        return false;
    moveCaret(kind, extend);
    return true;
}

void DiffCanvas::seedCaret(int file, bool old, int row)
{
    TextMark mark;
    mark.file = file;
    mark.row = row;
    mark.old = old;
    mark.code = true;
    m_anchor = mark;
    m_caret = mark;
    m_goal = 0;
}

void DiffCanvas::viewEdge(int *top, int *bottom) const
{
    const int scroll = verticalScrollBar()->value();
    *top = scroll + textTop();
    *bottom = scroll + qMax(1, viewport()->height());
}

bool DiffCanvas::rowShown(int file, int row) const
{
    int top = 0;
    int bottom = 0;
    viewEdge(&top, &bottom);
    for (const Band &band : m_bands) {
        if (band.kind != Band::Row || band.file != file || band.row != row)
            continue;
        return band.y + band.h > top && band.y < bottom;
    }
    return false;
}

int DiffCanvas::shownRow(int file, bool old, int first, int last) const
{
    int top = 0;
    int bottom = 0;
    viewEdge(&top, &bottom);
    for (const Band &band : m_bands) {
        if (band.y >= bottom)
            break;
        if (band.kind != Band::Row || band.file != file || band.y + band.h <= top)
            continue;
        if (band.row < first || band.row > last)
            continue;
        const DiffRow &line = m_doc.files.at(file).rows.at(band.row);
        if ((old ? line.leftNum : line.rightNum) > 0)
            return band.row;
    }
    return -1;
}

int DiffCanvas::caretRow(int file, bool old, int first, int last) const
{
    if (m_selFile == file && m_selRow >= first && m_selRow <= last && rowShown(file, m_selRow)) {
        const DiffRow &line = m_doc.files.at(file).rows.at(m_selRow);
        if ((old ? line.leftNum : line.rightNum) > 0)
            return m_selRow;
    }
    const int shown = shownRow(file, old, first, last);
    return shown >= 0 ? shown : first;
}

bool DiffCanvas::placeCaret()
{
    if (m_anchor.file >= 0 && m_anchor.code && m_anchor.file == m_caret.file && m_anchor.old == m_caret.old)
        return true;
    const int file = paneFile();
    if (file < 0 || file >= m_doc.files.size())
        return false;
    const bool old = paneOld(file);
    int first = 0;
    int last = 0;
    if (!sideEnd(file, old, &first, &last, nullptr))
        return false;
    seedCaret(file, old, caretRow(file, old, first, last));
    return true;
}

bool DiffCanvas::collapseMove(bool forward)
{
    TextMark from;
    TextMark to;
    if (!markEnds(&from, &to))
        return false;
    m_caret = forward ? to : from;
    m_anchor = m_caret;
    m_goal = m_caret.pos;
    return true;
}

void DiffCanvas::applyMove(int kind)
{
    const int dir = kind > 0 ? 1 : -1;
    const int which = qAbs(kind);
    if (which == MoveNextChar)
        stepChar(dir);
    else if (which == MoveNextLine)
        stepLine(dir);
    else if (which == MoveNextWord)
        stepWord(dir);
    else if (which == MoveLineEnd)
        stepEdge(kind > 0);
    else if (which == MoveDocEnd)
        stepDoc(kind > 0);
}

void DiffCanvas::moveCaret(int kind, bool extend)
{
    dropDrag();
    if (!extend && collapseMove(kind > 0)) {
        followCaret();
        return;
    }
    if (!extend)
        m_anchor = m_caret;
    applyMove(kind);
    if (!extend)
        m_anchor = m_caret;
    followCaret();
}
