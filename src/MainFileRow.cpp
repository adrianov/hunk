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
                   const QString &name)
{
    painter->save();
    painter->setClipRect(textRect);
    painter->setFont(opt.font);
    const QString shown =
        opt.fontMetrics.elidedText(name, Qt::ElideRight, qMax(0, textRect.width() - statWidth(opt.fontMetrics, index)));
    int x = textRect.left();
    drawChunk(painter, &x, textRect, shown, nameInk(opt, index), opt.fontMetrics);
    drawStats(painter, &x, textRect, index, opt.fontMetrics);
    painter->restore();
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
        const QBrush brush = index.data(Qt::BackgroundRole).value<QBrush>();
        if (brush.style() == Qt::NoBrush || opt.state.testFlag(QStyle::State_Selected)) {
            opt.text.clear();
            opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        } else {
            painter->fillRect(opt.rect, brush);
        }
        paintFileText(painter, opt, textRect, index, name);
    }
};

} // namespace

void installSeenRows(QTreeWidget *tree)
{
    tree->setItemDelegate(new SeenRow(tree));
}
