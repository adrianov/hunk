#include "DiffCanvas.hpp"

#include "DiffColors.hpp"
#include "DiffSyntax.hpp"

#include <QPainter>

namespace {

struct SideColors {
    QColor bg;
    QColor fg;
    QColor word;
    QColor border;
    bool mark = false;
};

SideColors colorsFor(SideStyle style)
{
    SideColors colors{kBg, kText, kAddWord, kAddFg, false};
    if (style == SideStyle::Empty)
        colors.bg = kEmpty;
    else if (style == SideStyle::Add)
        colors = SideColors{kAddBg, kAddFg, kAddWord, kAddFg, true};
    else if (style == SideStyle::Del)
        colors = SideColors{kDelBg, kDelFg, kDelWord, kDelFg, true};
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

} // namespace

void DiffCanvas::paintOneSide(QPainter &painter, int cellX, int cellW, SideStyle style, const QString &text, int number,
                              const QList<Piece> &pieces, bool note, int top, int height)
{
    const SideColors colors = colorsFor(style);
    fillCell(painter, cellX, cellW, top, height, m_gutterW, colors);
    if (note)
        paintNoteDot(painter, cellX, top, m_rowH);
    paintNumber(painter, m_mono, cellX, top, m_rowH, m_gutterW, number);
    paintWrapped(painter, m_mono, cellX, cellW, m_gutterW, top, height, m_rowH, text, pieces, colors.fg,
                 colors.mark ? colors.word : QColor());
}
