#include "DiffCanvas.hpp"

#include "DiffColors.hpp"

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

void paintSpans(QPainter &painter, int x, int baseline, int top, int height, const QString &text,
                const QList<WordSpan> &spans, const QColor &color, const QColor &wordBg)
{
    const QFontMetrics metrics(painter.font());
    if (spans.isEmpty()) {
        painter.setPen(color);
        painter.drawText(x, baseline, text);
        return;
    }
    for (const WordSpan &span : spans) {
        const QString fragment = text.mid(span.start, span.end - span.start);
        const int width = metrics.horizontalAdvance(fragment);
        if (span.changed)
            painter.fillRect(x, top, width, height, wordBg);
        painter.setPen(color);
        painter.drawText(x, baseline, fragment);
        x += width;
    }
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

} // namespace

void DiffCanvas::paintOneSide(QPainter &painter, int cellX, int cellW, SideStyle style, const QString &text, int number,
                              const QList<WordSpan> &spans, bool note, int top, int height, int baseline, int scrollX)
{
    const SideColors colors = colorsFor(style);
    fillCell(painter, cellX, cellW, top, height, m_gutterW, colors);
    if (note)
        paintNoteDot(painter, cellX, top, height);
    paintNumber(painter, m_mono, cellX, top, height, m_gutterW, number);
    painter.save();
    painter.setClipRect(cellX + m_gutterW, top, qMax(1, cellW - m_gutterW), height);
    painter.setFont(m_mono);
    paintSpans(painter, cellX + m_gutterW + 8 - scrollX, baseline, top, height, text, spans, colors.fg,
               colors.mark ? colors.word : QColor());
    painter.restore();
}
