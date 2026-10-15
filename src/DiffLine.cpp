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
    return true;
}

DiffCanvas::TextMark DiffCanvas::clickMark(const QMouseEvent *mouse) const
{
    return textAt(int(mouse->position().x()), int(mouse->position().y()) + verticalScrollBar()->value());
}

void DiffCanvas::pickWord(QMouseEvent *mouse)
{
    if (pressIgnored(mouse) || evenSplit(mouse) || headerPress(mouse))
        return;
    const TextMark hit = clickMark(mouse);
    if (!hit.code || lineMarked(hit))
        return;
    takeWord(hit);
    endText(mouse);
}
