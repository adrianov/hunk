// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffColors.hpp"
#include "DiffSyntax.hpp"

#include <QFontMetrics>
#include <QPainter>

namespace {

struct SideColors {
    QColor bg;
    QColor fg;
    QColor word;
    QColor border;
    bool mark = false;
};

QColor tone(QColor color, RowHeat heat)
{
    if (heat == RowHeat::Seen)
        color.setAlpha(color.alpha() / 2);
    else if (heat == RowHeat::Fresh)
        color.setAlpha(qMin(160, color.alpha() * 3 / 2));
    return color;
}

QColor barColor(const QColor &color, RowHeat heat)
{
    if (heat != RowHeat::Seen)
        return color;
    QColor bar = color;
    bar.setAlpha(110);
    return bar;
}

SideColors colorsFor(SideStyle style, RowHeat heat)
{
    SideColors colors{kBg, kText, kAddWord, kAddFg, false};
    if (style == SideStyle::Empty)
        colors.bg = kEmpty;
    else if (style == SideStyle::Add)
        colors = SideColors{tone(kAddBg, heat), kAddFg, tone(kAddWord, heat), barColor(kAddFg, heat), true};
    else if (style == SideStyle::Del)
        colors = SideColors{tone(kDelBg, heat), kDelFg, tone(kDelWord, heat), barColor(kDelFg, heat), true};
    return colors;
}

void fillCell(QPainter &painter, int cellX, int cellW, int top, int height, int gutterW, const SideColors &colors)
{
    painter.fillRect(cellX, top, cellW, height, colors.bg);
    painter.fillRect(cellX, top, gutterW, height, kGutter);
    if (colors.mark)
        painter.fillRect(cellX + gutterW, top, 3, height, colors.border);
}

void paintNoteDot(QPainter &painter, int cellX, int top, int height)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(kNote);
    painter.drawEllipse(QPointF(cellX + 8, top + height / 2.0), 3.5, 3.5);
    painter.setBrush(Qt::NoBrush);
}

void paintNumber(QPainter &painter, const QFont &font, int cellX, int top, int height, int gutterW, int number)
{
    if (number <= 0)
        return;
    painter.setFont(font);
    painter.setPen(kMuted);
    painter.drawText(QRect(cellX + 14, top, gutterW - 20, height), Qt::AlignRight | Qt::AlignVCenter, QString::number(number));
}

void paintWrapped(QPainter &painter, const QFont &font, int cellX, int cellW, int gutter, int top, int height,
                  int lineH, const QString &text, const QList<Piece> &pieces, const QColor &color, const QColor &word)
{
    const int codeW = cellW - gutter;
    painter.save();
    painter.setClipRect(cellX + gutter, top, codeW > 0 ? codeW : 1, height);
    painter.setFont(font);
    paintCode(painter, cellX + gutter + 8, top, lineH, text, pieces, color, word);
    painter.restore();
}

void fillPiece(QPainter &painter, const QFontMetrics &metrics, const QString &text, const Piece &piece, int *drawX,
               int top, int lineH, int from, int to, const QColor &color)
{
    if (piece.end <= piece.start)
        return;
    const int width = metrics.horizontalAdvance(text.mid(piece.start, piece.end - piece.start));
    const int left = qMax(piece.start, from);
    const int right = qMin(piece.end, to);
    if (left < right) {
        const int skip = metrics.horizontalAdvance(text.mid(piece.start, left - piece.start));
        const int span = metrics.horizontalAdvance(text.mid(left, right - left));
        painter.fillRect(*drawX + skip, top, qMax(1, span), lineH, color);
    }
    *drawX += width;
}

void paintRuns(QPainter &painter, const QFontMetrics &metrics, const QString &text, const QList<Piece> &pieces, int x,
               int top, int lineH, int from, int to, const QColor &color)
{
    int lineTop = top;
    int drawX = x;
    bool lined = false;
    for (const Piece &piece : pieces) {
        if (piece.newLine) {
            if (lined)
                lineTop += lineH;
            lined = true;
            drawX = x;
        }
        fillPiece(painter, metrics, text, piece, &drawX, lineTop, lineH, from, to, color);
    }
}

} // namespace

void DiffCanvas::paintTextMark(QPainter &painter, int cellX, int cellW, int top, int height, int file, int row,
                               bool oldSide, const QString &text, const QList<Piece> &pieces)
{
    int start = 0;
    int end = 0;
    if (!markSpan(file, row, oldSide, text.size(), &start, &end))
        return;
    QColor color = kAccent;
    color.setAlpha(140);
    painter.save();
    const int codeW = cellW - m_gutterW;
    painter.setClipRect(cellX + m_gutterW, top, codeW > 0 ? codeW : 1, height);
    painter.setPen(Qt::NoPen);
    paintRuns(painter, QFontMetrics(m_mono), text, pieces, cellX + m_gutterW + 8, top, m_rowH, start, end, color);
    painter.restore();
}

void DiffCanvas::paintOneSide(QPainter &painter, int cellX, int cellW, SideStyle style, RowHeat heat, const QString &text,
                              int number, const QList<Piece> &pieces, bool note, int top, int height, int file, int row,
                              bool oldSide)
{
    const SideColors colors = colorsFor(style, heat);
    fillCell(painter, cellX, cellW, top, height, m_gutterW, colors);
    paintTextMark(painter, cellX, cellW, top, height, file, row, oldSide, text, pieces);
    if (note)
        paintNoteDot(painter, cellX, top, m_rowH);
    paintNumber(painter, m_mono, cellX, top, m_rowH, m_gutterW, number);
    paintWrapped(painter, m_mono, cellX, cellW, m_gutterW, top, height, m_rowH, text, pieces, colors.fg,
                 colors.mark ? colors.word : QColor());
}
