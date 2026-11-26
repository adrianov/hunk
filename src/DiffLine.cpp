// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QDateTime>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyleHints>

namespace {

QString rowText(const DiffRow &row, bool old)
{
    return old ? row.leftText : row.rightText;
}

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

} // namespace

bool DiffCanvas::wordFresh() const
{
    if (m_wordMs <= 0)
        return false;
    return QDateTime::currentMSecsSinceEpoch() - m_wordMs
        <= QGuiApplication::styleHints()->mouseDoubleClickInterval();
}

bool DiffCanvas::onWord(const TextMark &hit) const
{
    if (!wordFresh() || hit.file != m_anchor.file || hit.row != m_anchor.row || hit.old != m_anchor.old)
        return false;
    return m_caret.file == hit.file && m_caret.row == hit.row && m_caret.old == hit.old;
}

bool DiffCanvas::lineMarked(const TextMark &hit) const
{
    if (!onWord(hit))
        return false;
    const QString text = rowText(m_doc.files.at(hit.file).rows.at(hit.row), hit.old);
    return !text.isEmpty() && m_anchor.pos == 0 && m_caret.pos == text.size();
}

void DiffCanvas::markLine(const TextMark &hit, int size)
{
    m_anchor = hit;
    m_caret = hit;
    m_anchor.pos = 0;
    m_caret.pos = size;
    m_selFile = hit.file;
    m_selRow = hit.row;
    m_selOld = hit.old;
    m_goal = -1;
    m_wordMs = 0;
    viewport()->update();
}

bool DiffCanvas::takeLine(int x, int y)
{
    const TextMark hit = textAt(x, y);
    if (!hit.code || !onWord(hit))
        return false;
    const QString text = rowText(m_doc.files.at(hit.file).rows.at(hit.row), hit.old);
    if (text.isEmpty())
        return false;
    markLine(hit, text.size());
    holdText();
    return true;
}

DiffCanvas::TextMark DiffCanvas::clickMark(const QMouseEvent *mouse) const
{
    return textAt(int(mouse->position().x()), int(mouse->position().y()) + verticalScrollBar()->value());
}

void DiffCanvas::takeWord(const TextMark &hit)
{
    int from = 0;
    int to = 0;
    widenWord(rowText(m_doc.files.at(hit.file).rows.at(hit.row), hit.old), hit.pos, &from, &to);
    m_anchor = hit;
    m_caret = hit;
    m_anchor.pos = from;
    m_caret.pos = to;
    m_selFile = hit.file;
    m_selRow = hit.row;
    m_selOld = hit.old;
    m_goal = -1;
    m_wordFrom = from;
    m_wordTo = to;
    m_wordMs = QDateTime::currentMSecsSinceEpoch();
    viewport()->update();
}

// Keep the double-clicked word selected and grow by whole words under the pointer.
void DiffCanvas::stretchWord(TextMark *hit)
{
    const QString text = rowText(m_doc.files.at(hit->file).rows.at(hit->row), hit->old);
    int from = 0;
    int to = 0;
    widenWord(text, hit->pos, &from, &to);
    TextMark pivot = *hit;
    pivot.row = m_selRow;
    pivot.code = true;
    if (hit->row > m_selRow || (hit->row == m_selRow && hit->pos >= m_wordFrom)) {
        pivot.pos = m_wordFrom;
        m_anchor = pivot;
        hit->pos = hit->row == m_selRow ? qMax(to, m_wordTo) : to;
        return;
    }
    pivot.pos = m_wordTo;
    m_anchor = pivot;
    hit->pos = hit->row == m_selRow ? qMin(from, m_wordFrom) : from;
}

void DiffCanvas::pickWord(QMouseEvent *mouse)
{
    if (pressIgnored(mouse) || evenSplit(mouse) || headerPress(mouse))
        return;
    const TextMark hit = clickMark(mouse);
    if (!hit.code || lineMarked(hit))
        return;
    takeWord(hit);
    holdText();
}
