// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

namespace {

const QKeyEvent *keyOf(const QEvent *event)
{
    const auto type = event->type();
    if (type != QEvent::KeyPress && type != QEvent::KeyRelease)
        return nullptr;
    return static_cast<const QKeyEvent *>(event);
}

bool modKey(const QKeyEvent *key)
{
    if (key->isAutoRepeat())
        return false;
    const int code = key->key();
    return code == Qt::Key_Control || code == Qt::Key_Meta;
}

int cutAt(const QFontMetrics &metrics, const QString &text, int start, int end, int local)
{
    int pos = start;
    while (pos < end) {
        const int half = (metrics.horizontalAdvance(text.mid(start, pos - start))
                          + metrics.horizontalAdvance(text.mid(start, pos + 1 - start)))
                         / 2;
        if (local < half)
            break;
        ++pos;
    }
    return pos;
}

int charAt(const QString &text, const QList<Piece> &pieces, int localX, int localY, const QFont &font, int lineH)
{
    const QFontMetrics metrics(font);
    const int target = localY < 0 ? 0 : localY / lineH;
    int line = -1;
    int drawX = 0;
    int at = 0;
    for (const Piece &piece : pieces) {
        if (piece.newLine) {
            if (line == target)
                return at;
            ++line;
            drawX = 0;
            at = piece.start;
        }
        if (line != target || piece.end <= piece.start)
            continue;
        const int width = metrics.horizontalAdvance(text.mid(piece.start, piece.end - piece.start));
        if (localX < drawX + width)
            return cutAt(metrics, text, piece.start, piece.end, localX - drawX);
        drawX += width;
        at = piece.end;
    }
    return at;
}

} // namespace

void DiffCanvas::clearMark()
{
    m_dragTimer.stop();
    if (m_dragText)
        viewport()->releaseMouse();
    m_dragText = false;
    m_anchor = {};
    m_caret = {};
    m_goal = -1;
    m_wordMs = 0;
}

DiffCanvas::TextMark DiffCanvas::codeAt(TextMark mark, const DiffRow &row, int localX, int localY) const
{
    const QString &side = mark.old ? row.leftText : row.rightText;
    const QList<Piece> &pieces = mark.old ? row.leftPiece : row.rightPiece;
    mark.pos = charAt(side, pieces, localX, localY, m_mono, m_rowH);
    mark.code = true;
    return mark;
}

DiffCanvas::TextMark DiffCanvas::textAt(int x, int y) const
{
    TextMark mark;
    const Band *band = bandAt(y);
    if (!band || band->kind != Band::Row)
        return mark;
    const FileDiff &file = m_doc.files.at(band->file);
    bool left = false;
    const int origin = sideOrigin(file.singlePane(), x, &left);
    mark.file = band->file;
    mark.row = band->row;
    mark.old = left;
    if (x - origin < m_gutterW)
        return mark;
    return codeAt(mark, file.rows.at(band->row), x - origin - m_gutterW - 8, y - band->y);
}

void DiffCanvas::beginText(int x, int y, bool extend)
{
    m_wordMs = 0;
    const TextMark hit = textAt(x, y);
    if (!hit.code)
        return;
    m_selFile = hit.file;
    m_selRow = hit.row;
    m_selOld = hit.old;
    if (!extend || m_anchor.file != hit.file || m_anchor.old != hit.old)
        m_anchor = hit;
    m_caret = hit;
    m_goal = -1;
    m_dragText = true;
    viewport()->grabMouse();
    viewport()->setCursor(Qt::IBeamCursor);
    viewport()->update();
}

void DiffCanvas::dragText(int x, int y)
{
    const TextMark hit = textAt(x, y);
    if (!hit.code || hit.file != m_anchor.file || hit.old != m_anchor.old)
        return;
    if (hit.row == m_caret.row && hit.pos == m_caret.pos)
        return;
    m_caret = hit;
    m_goal = -1;
    viewport()->update();
}

void DiffCanvas::endText(QMouseEvent *mouse)
{
    if (!m_dragText || mouse->button() != Qt::LeftButton)
        return;
    m_dragTimer.stop();
    m_dragText = false;
    viewport()->releaseMouse();
}

void DiffCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        stopDef();
        if (m_anchor.file >= 0) {
            clearMark();
            viewport()->update();
        }
        return;
    }
    QAbstractScrollArea::keyPressEvent(event);
}

bool DiffCanvas::eventFilter(QObject *watched, QEvent *event)
{
    const QKeyEvent *key = keyOf(event);
    if (!key)
        return QAbstractScrollArea::eventFilter(watched, event);
    if (event->type() == QEvent::KeyPress && paneKey(key))
        return true;
    if (modKey(key))
        QTimer::singleShot(0, this, &DiffCanvas::placeCursor);
    return QAbstractScrollArea::eventFilter(watched, event);
}
