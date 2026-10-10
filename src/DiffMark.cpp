// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffFold.hpp"

#include <QMouseEvent>
#include <QScrollBar>
#include <QStringList>

namespace {

bool wordChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

int wordStart(const QString &text, int at)
{
    while (at > 0 && wordChar(text.at(at - 1)))
        --at;
    return at;
}

int wordEnd(const QString &text, int at)
{
    int end = at + 1;
    while (end < text.size() && wordChar(text.at(end)))
        ++end;
    return end;
}

int wordPos(const QString &text, int pos)
{
    int at = qBound(0, pos, int(text.size()));
    if (at > 0 && (at == text.size() || !wordChar(text.at(at))))
        --at;
    return at;
}

void widenWord(const QString &text, int pos, int *from, int *to)
{
    const int at = wordPos(text, pos);
    if (text.isEmpty() || !wordChar(text.at(at))) {
        *from = at;
        *to = qMin(at + 1, int(text.size()));
        return;
    }
    *from = wordStart(text, at);
    *to = wordEnd(text, at);
}

QString sideLine(const DiffRow &row, bool old)
{
    return old ? row.leftText : row.rightText;
}

bool sliceOf(int fromRow, int fromPos, int toRow, int toPos, int row, int size, int *start, int *end)
{
    if (row < fromRow || row > toRow)
        return false;
    *start = row == fromRow ? qBound(0, fromPos, size) : 0;
    *end = row == toRow ? qBound(*start, toPos, size) : size;
    return *start < *end;
}

bool insideFold(const QList<FoldSpan> &hidden, int row)
{
    for (const FoldSpan &span : hidden) {
        if (row >= span.first && row <= span.last)
            return true;
    }
    return false;
}

QString lineSlice(const DiffRow &row, bool old, int rowIndex, int fromRow, int fromPos, int toRow, int toPos)
{
    const QString text = sideLine(row, old);
    int start = 0;
    int end = 0;
    if (!sliceOf(fromRow, fromPos, toRow, toPos, rowIndex, text.size(), &start, &end))
        return {};
    return text.mid(start, end - start);
}

void appendMarked(QStringList *lines, const FileDiff &file, const QList<FoldSpan> &hidden, int row, bool old,
                  int fromRow, int fromPos, int toRow, int toPos)
{
    if (insideFold(hidden, row))
        return;
    lines->append(lineSlice(file.rows.at(row), old, row, fromRow, fromPos, toRow, toPos));
}

} // namespace

void DiffCanvas::takeWord(const TextMark &hit)
{
    int from = 0;
    int to = 0;
    widenWord(sideLine(m_doc.files.at(hit.file).rows.at(hit.row), hit.old), hit.pos, &from, &to);
    m_anchor = hit;
    m_caret = hit;
    m_anchor.pos = from;
    m_caret.pos = to;
    m_selFile = hit.file;
    m_selRow = hit.row;
    m_selOld = hit.old;
}

void DiffCanvas::pickWord(QMouseEvent *mouse)
{
    if (pressIgnored(mouse) || evenSplit(mouse) || headerPress(mouse))
        return;
    const TextMark hit = textAt(int(mouse->position().x()), int(mouse->position().y()) + verticalScrollBar()->value());
    if (!hit.code)
        return;
    takeWord(hit);
    endText(mouse);
}

bool DiffCanvas::markEnds(TextMark *from, TextMark *to) const
{
    if (m_anchor.file < 0 || m_anchor.file != m_caret.file || m_anchor.old != m_caret.old)
        return false;
    *from = m_anchor;
    *to = m_caret;
    if (from->row > to->row || (from->row == to->row && from->pos > to->pos)) {
        const TextMark swap = *from;
        *from = *to;
        *to = swap;
    }
    return from->row != to->row || from->pos != to->pos;
}

bool DiffCanvas::markedRows(int file, int row, bool oldSide, int *from, int *to) const
{
    TextMark start;
    TextMark stop;
    if (!markEnds(&start, &stop) || start.file != file || start.old != oldSide)
        return false;
    if (row < start.row || row > stop.row || start.row == stop.row)
        return false;
    *from = start.row;
    *to = stop.row;
    return true;
}

bool DiffCanvas::markSpan(int file, int row, bool oldSide, int size, int *start, int *end) const
{
    TextMark from;
    TextMark to;
    if (!markEnds(&from, &to) || file != from.file || oldSide != from.old)
        return false;
    return sliceOf(from.row, from.pos, to.row, to.pos, row, size, start, end);
}

QString DiffCanvas::markedText() const
{
    TextMark from;
    TextMark to;
    if (!markEnds(&from, &to))
        return {};
    const FileDiff &file = m_doc.files.at(from.file);
    const QList<FoldSpan> hidden = foldSpans(file.rows, pinnedRows(from.file));
    QStringList lines;
    for (int row = from.row; row <= to.row; ++row)
        appendMarked(&lines, file, hidden, row, from.old, from.row, from.pos, to.row, to.pos);
    return lines.join(QLatin1Char('\n'));
}

