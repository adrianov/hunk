// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffColors.hpp"
#include "MainSeen.hpp"

#include <QFontMetrics>
#include <QPainter>

namespace {

struct StatText {
    QString plus;
    QString minus;
    int plusW = 0;
    int minusW = 0;
};

StatText measureStats(const QFontMetrics &metrics, const FileDiff &file)
{
    StatText stat;
    stat.plus = QStringLiteral("+") + QString::number(file.adds);
    stat.minus = QStringLiteral("−") + QString::number(file.dels);
    stat.plusW = metrics.horizontalAdvance(stat.plus);
    stat.minusW = metrics.horizontalAdvance(stat.minus);
    return stat;
}

int headerBaseline(const QRect &rect, const QFontMetrics &metrics)
{
    return rect.top() + (rect.height() - metrics.height()) / 2 + metrics.ascent();
}

void drawStats(QPainter &painter, const QRect &rect, int baseline, const StatText &stat)
{
    painter.setPen(kDelFg);
    painter.drawText(rect.right() - 12 - stat.minusW, baseline, stat.minus);
    painter.setPen(kAddFg);
    painter.drawText(rect.right() - 24 - stat.minusW - stat.plusW, baseline, stat.plus);
}

constexpr int kCheckX = 10;
constexpr int kCheckSide = 22;
constexpr int kTitleX = kCheckX + kCheckSide + 8;

void drawTitle(QPainter &painter, const QRect &rect, const QFontMetrics &metrics, const QString &title, int statsW)
{
    painter.setPen(kFile);
    painter.drawText(rect.adjusted(kTitleX, 0, -statsW, 0), Qt::AlignVCenter | Qt::AlignLeft,
                     metrics.elidedText(title, Qt::ElideMiddle, qMax(20, rect.width() - statsW - kTitleX - 4)));
}

void paintLabelBar(QPainter &painter, int y, int height, int viewW)
{
    painter.fillRect(0, y, viewW, height, kGutter);
    painter.setPen(kLine);
    painter.drawLine(0, y + height - 1, viewW, y + height - 1);
}

void paintPairLabels(QPainter &painter, int y, int height, int leftW, int viewW, const QString &left,
                     const QString &right)
{
    const int rightW = viewW - leftW;
    painter.drawText(QRect(12, y, leftW - 16, height), Qt::AlignVCenter | Qt::AlignLeft, left);
    painter.drawText(QRect(leftW + 12, y, rightW - 16, height), Qt::AlignVCenter | Qt::AlignLeft, right);
    painter.setPen(kLine);
    painter.drawLine(leftW, y, leftW, y + height);
}

} // namespace

QRect DiffCanvas::titleCheck(const QRect &header) const
{
    return QRect(header.left() + kCheckX, header.top() + (header.height() - kCheckSide) / 2, kCheckSide, kCheckSide);
}

void DiffCanvas::paintFileHeader(QPainter &painter, const QRect &rect, const FileDiff &file, const QFont &font, bool hot)
{
    painter.fillRect(rect, kHeaderBg);
    painter.fillRect(rect.left(), rect.top(), 3, rect.height(), kAccent);
    painter.setFont(font);
    const QFontMetrics metrics(font);
    const StatText stat = measureStats(metrics, file);
    drawStats(painter, rect, headerBaseline(rect, metrics), stat);
    paintReviewCheck(&painter, titleCheck(rect), hot, false, kText);
    drawTitle(painter, rect, metrics, file.title(), stat.plusW + stat.minusW + 28);
}

void DiffCanvas::paintHeaderBand(QPainter &painter, const Band &band, const FileDiff &file, int viewW)
{
    QFont headerFont = font();
    headerFont.setBold(true);
    paintFileHeader(painter, QRect(0, band.y, viewW, band.h), file, headerFont, band.file == m_checkFile);
}

void DiffCanvas::paintLabels(QPainter &painter, const Band &band, const FileDiff &file, int viewW)
{
    paintLabelBar(painter, band.y, band.h, viewW);
    painter.setPen(kMuted);
    painter.setFont(font());
    if (file.singlePane()) {
        painter.drawText(QRect(12, band.y, viewW - 16, band.h), Qt::AlignVCenter | Qt::AlignLeft,
                         m_rightLabel + QStringLiteral(" (new file)"));
        return;
    }
    paintPairLabels(painter, band.y, band.h, pairSplit(viewW), viewW, m_leftLabel, m_rightLabel);
}

void DiffCanvas::paintNoteBand(QPainter &painter, const Band &band, const FileDiff &file, int viewW)
{
    painter.setPen(kMuted);
    painter.setFont(font());
    const QString text = file.binary ? QStringLiteral("Binary file") : QStringLiteral("No line changes");
    painter.drawText(QRect(12, band.y, viewW - 16, band.h), Qt::AlignVCenter | Qt::AlignLeft, text);
}
