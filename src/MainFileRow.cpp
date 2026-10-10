// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainSeen.hpp"

#include "DiffColors.hpp"

#include <QPainter>
#include <QStyledItemDelegate>
#include <QTreeWidget>

namespace {

QColor nameInk(const QStyleOptionViewItem &opt, const QModelIndex &index)
{
    if (opt.state.testFlag(QStyle::State_Selected))
        return opt.palette.color(QPalette::HighlightedText);
    const QVariant ink = index.data(Qt::ForegroundRole);
    if (ink.canConvert<QBrush>() && ink.value<QBrush>().style() != Qt::NoBrush)
        return ink.value<QBrush>().color();
    return opt.palette.color(QPalette::Text);
}

void drawChunk(QPainter *painter, int *x, const QRect &row, const QString &text, const QColor &color,
               const QFontMetrics &metrics)
{
    const int width = metrics.horizontalAdvance(text);
    painter->setPen(color);
    painter->drawText(QRect(*x, row.top(), width, row.height()), Qt::AlignVCenter | Qt::AlignLeft, text);
    *x += width;
}

void drawStats(QPainter *painter, int *x, const QRect &row, const QModelIndex &index, const QFontMetrics &metrics)
{
    const QVariant adds = index.data(Qt::UserRole + 1);
    if (!adds.isValid())
        return;
    drawChunk(painter, x, row, QStringLiteral("    +%1").arg(adds.toInt()), kAddFg, metrics);
    drawChunk(painter, x, row, QStringLiteral("  −%1").arg(index.data(Qt::UserRole + 2).toInt()), kDelFg, metrics);
}

int statWidth(const QFontMetrics &metrics, const QModelIndex &index)
{
    const QVariant adds = index.data(Qt::UserRole + 1);
    if (!adds.isValid())
        return 0;
    return metrics.horizontalAdvance(
        QStringLiteral("    +%1  −%2").arg(adds.toInt()).arg(index.data(Qt::UserRole + 2).toInt()));
}

void paintFileText(QPainter *painter, const QStyleOptionViewItem &opt, const QRect &textRect, const QModelIndex &index,
                   const QString &name, int reserve)
{
    painter->save();
    painter->setClipRect(textRect.adjusted(0, 0, -reserve, 0));
    painter->setFont(opt.font);
    const QString shown = opt.fontMetrics.elidedText(
        name, Qt::ElideRight, qMax(0, textRect.width() - statWidth(opt.fontMetrics, index) - reserve));
    int x = textRect.left();
    drawChunk(painter, &x, textRect, shown, nameInk(opt, index), opt.fontMetrics);
    drawStats(painter, &x, textRect, index, opt.fontMetrics);
    painter->restore();
}

bool checkHot(const QStyleOptionViewItem &opt, const QModelIndex &index)
{
    return opt.widget && opt.widget->property("checkHot").value<quintptr>() == quintptr(index.internalPointer());
}

void paintCheck(QPainter *painter, const QStyleOptionViewItem &opt, const QRect &rect, bool hot)
{
    paintReviewCheck(painter, rect, hot, opt.state.testFlag(QStyle::State_Selected),
                     opt.palette.color(QPalette::HighlightedText));
}

void paintRowFace(QPainter *painter, QStyleOptionViewItem *opt, const QModelIndex &index)
{
    const QBrush brush = index.data(Qt::BackgroundRole).value<QBrush>();
    if (brush.style() == Qt::NoBrush || opt->state.testFlag(QStyle::State_Selected)) {
        opt->text.clear();
        opt->widget->style()->drawControl(QStyle::CE_ItemViewItem, opt, painter, opt->widget);
        return;
    }
    painter->fillRect(opt->rect, brush);
}

class SeenRow : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        const QRect textRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        const QString name = opt.text;
        const bool file = index.data(Qt::UserRole).toInt() >= 0;
        const QRect check = fileCheckRect(opt.rect);
        paintRowFace(painter, &opt, index);
        paintFileText(painter, opt, textRect, index, name, file ? check.width() + 8 : 0);
        if (file)
            paintCheck(painter, opt, check, checkHot(opt, index));
    }
};

} // namespace

void paintReviewCheck(QPainter *painter, const QRect &rect, bool hot, bool selected, const QColor &selectedInk)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QColor fill = kAccent;
    fill.setAlpha(hot && !selected ? 48 : 0);
    const QColor ink = selected ? selectedInk : (hot ? kText : kMuted);
    painter->setPen(selected ? ink : (hot ? kAccent : kLine));
    painter->setBrush(fill);
    painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), 4, 4);
    painter->setPen(ink);
    painter->drawText(rect, Qt::AlignCenter, QStringLiteral("✓"));
    painter->restore();
}

QRect fileCheckRect(const QRect &row)
{
    constexpr int side = 22;
    return QRect(row.right() - 8 - side, row.top() + (row.height() - side) / 2, side, side);
}

void installSeenRows(QTreeWidget *tree)
{
    tree->setItemDelegate(new SeenRow(tree));
}
