// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>

namespace {

struct PaneHit {
    bool left = false;
    int origin = 0;
};

PaneHit paneHit(bool single, int viewW, int x)
{
    const int paneW = single ? viewW : viewW / 2;
    PaneHit hit;
    hit.left = !single && x < paneW;
    hit.origin = hit.left || single ? 0 : paneW;
    return hit;
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
    if (m_dragText)
        viewport()->releaseMouse();
    m_dragText = false;
    m_anchor = {};
    m_caret = {};
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
    const PaneHit pane = paneHit(file.singlePane(), viewport()->width(), x);
    mark.file = band->file;
    mark.row = band->row;
    mark.old = pane.left;
    if (x - pane.origin < m_gutterW)
        return mark;
    return codeAt(mark, file.rows.at(band->row), x - pane.origin - m_gutterW - 8, y - band->y);
}

void DiffCanvas::beginText(int x, int y, bool extend)
{
    const TextMark hit = textAt(x, y);
    if (!hit.code)
        return;
    m_selFile = hit.file;
    m_selRow = hit.row;
    m_selOld = hit.old;
    if (!extend || m_anchor.file != hit.file || m_anchor.old != hit.old)
        m_anchor = hit;
    m_caret = hit;
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
    viewport()->update();
}

void DiffCanvas::endText(QMouseEvent *mouse)
{
    if (!m_dragText || mouse->button() != Qt::LeftButton)
        return;
    m_dragText = false;
    viewport()->releaseMouse();
}

void DiffCanvas::copyMarked()
{
    const QString text = markedText();
    if (text.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(text);
}

void DiffCanvas::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *copy = menu.addAction(QStringLiteral("Copy"));
    copy->setShortcut(QKeySequence::Copy);
    copy->setEnabled(!markedText().isEmpty());
    if (menu.exec(event->globalPos()) == copy)
        copyMarked();
    event->accept();
}

void DiffCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        copyMarked();
        return;
    }
    if (event->key() == Qt::Key_Escape && m_anchor.file >= 0) {
        clearMark();
        viewport()->update();
        return;
    }
    QAbstractScrollArea::keyPressEvent(event);
}
