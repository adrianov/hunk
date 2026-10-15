// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffFold.hpp"

#include <QScrollBar>

namespace {

bool wordChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

int wordNext(const QString &text, int pos)
{
    int at = qBound(0, pos, int(text.size()));
    if (at < text.size() && wordChar(text.at(at))) {
        while (at < text.size() && wordChar(text.at(at)))
            ++at;
    }
    while (at < text.size() && !wordChar(text.at(at)))
        ++at;
    return at;
}

int wordPrev(const QString &text, int pos)
{
    int at = qBound(0, pos, int(text.size()));
    if (at == 0)
        return 0;
    --at;
    while (at > 0 && !wordChar(text.at(at)))
        --at;
    while (at > 0 && wordChar(text.at(at - 1)))
        --at;
    return at;
}

bool rowFolded(const QList<FoldSpan> &hidden, int row)
{
    for (const FoldSpan &span : hidden) {
        if (row >= span.first && row <= span.last)
            return true;
    }
    return false;
}

bool onSide(const DiffRow &line, bool old)
{
    return (old ? line.leftNum : line.rightNum) > 0;
}

} // namespace

QString DiffCanvas::caretLine() const
{
    const DiffRow &line = m_doc.files.at(m_caret.file).rows.at(m_caret.row);
    return m_caret.old ? line.leftText : line.rightText;
}

int DiffCanvas::nearRow(int row, int dir) const
{
    const FileDiff &file = m_doc.files.at(m_caret.file);
    const QList<FoldSpan> hidden = foldSpans(file.rows, pinnedRows(m_caret.file));
    for (int next = row + dir; next >= 0 && next < file.rows.size(); next += dir) {
        if (!onSide(file.rows.at(next), m_caret.old) || rowFolded(hidden, next))
            continue;
        return next;
    }
    return -1;
}

void DiffCanvas::stepChar(int dir)
{
    if (dir > 0 && m_caret.pos < caretLine().size())
        ++m_caret.pos;
    else if (dir < 0 && m_caret.pos > 0)
        --m_caret.pos;
    else {
        const int row = nearRow(m_caret.row, dir);
        if (row < 0)
            return;
        m_caret.row = row;
        m_caret.pos = dir > 0 ? 0 : caretLine().size();
    }
    m_goal = m_caret.pos;
}

void DiffCanvas::stepWord(int dir)
{
    const int pos = dir > 0 ? wordNext(caretLine(), m_caret.pos) : wordPrev(caretLine(), m_caret.pos);
    if (pos != m_caret.pos) {
        m_caret.pos = pos;
        m_goal = pos;
        return;
    }
    const int row = nearRow(m_caret.row, dir);
    if (row < 0)
        return;
    m_caret.row = row;
    m_caret.pos = dir > 0 ? 0 : wordPrev(caretLine(), caretLine().size());
    m_goal = m_caret.pos;
}

void DiffCanvas::stepLine(int dir)
{
    if (m_goal < 0)
        m_goal = m_caret.pos;
    const int row = nearRow(m_caret.row, dir);
    if (row < 0)
        return;
    m_caret.row = row;
    m_caret.pos = qMin(m_goal, int(caretLine().size()));
}

void DiffCanvas::stepEdge(bool end)
{
    m_caret.pos = end ? caretLine().size() : 0;
    m_goal = m_caret.pos;
}

void DiffCanvas::stepDoc(bool end)
{
    int first = 0;
    int last = 0;
    int stop = 0;
    if (!sideEnd(m_caret.file, m_caret.old, &first, &last, &stop))
        return;
    m_caret.row = end ? last : first;
    m_caret.pos = end ? stop : 0;
    m_goal = m_caret.pos;
}

const DiffCanvas::Band *DiffCanvas::caretBand() const
{
    for (const Band &band : m_bands) {
        if (band.kind == Band::Row && band.file == m_caret.file && band.row == m_caret.row)
            return &band;
    }
    return nullptr;
}

void DiffCanvas::scrollBand(const Band &band)
{
    const int scroll = verticalScrollBar()->value();
    const int pad = barHeads(band.file) ? 0 : m_headerH;
    const int top = scroll + (headerStuck(scroll) ? pad : 0);
    const int viewH = viewport()->height();
    if (band.y >= top && band.y + m_rowH <= scroll + viewH)
        return;
    const int y = band.y < top ? band.y - pad : band.y + m_rowH - viewH;
    verticalScrollBar()->setValue(qMax(0, y));
}

void DiffCanvas::scrollCaret()
{
    if (const Band *band = caretBand())
        scrollBand(*band);
    m_selFile = m_caret.file;
    m_selRow = m_caret.row;
    m_selOld = m_caret.old;
    m_wordMs = 0;
    viewport()->update();
}

void DiffCanvas::followCaret()
{
    revealRow(m_caret.file, m_caret.row);
    scrollCaret();
}
