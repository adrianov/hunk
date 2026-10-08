// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainSeen.hpp"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeWidget>

namespace {

constexpr int seenVersion = 2;

QString seenKey(const QString &root)
{
    return QString::fromLatin1(QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Sha1).toHex());
}

bool keepSeen(QSettings *settings)
{
    if (settings->value(QStringLiteral("version")).toInt() == seenVersion)
        return true;
    settings->remove(QString());
    settings->setValue(QStringLiteral("version"), seenVersion);
    return false;
}

QHash<QString, QString> stampsFrom(const QByteArray &raw)
{
    QHash<QString, QString> seen;
    const QJsonObject object = QJsonDocument::fromJson(raw).object();
    for (auto it = object.begin(); it != object.end(); ++it)
        seen.insert(it.key(), it.value().toString());
    return seen;
}

void addField(QCryptographicHash *hash, const QByteArray &bytes)
{
    hash->addData(bytes);
    hash->addData(QByteArray(1, '\0'));
}

void hashRow(QCryptographicHash *hash, const DiffRow &row)
{
    if (row.kind == RowKind::Context)
        return;
    addField(hash, QByteArray::number(static_cast<int>(row.kind)));
    addField(hash, row.leftText.toUtf8());
    addField(hash, row.rightText.toUtf8());
}

} // namespace

QString fileStamp(const FileDiff &file)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    addField(&hash, file.path().toUtf8());
    addField(&hash, QByteArray::number(file.adds));
    addField(&hash, QByteArray::number(file.dels));
    addField(&hash, file.binary ? "1" : "0");
    for (const DiffRow &row : file.rows)
        hashRow(&hash, row);
    return QString::fromLatin1(hash.result().toHex());
}

QHash<QString, QString> readSeen(const QString &root)
{
    if (root.isEmpty())
        return {};
    QSettings settings;
    settings.beginGroup(QStringLiteral("seenFiles"));
    if (!keepSeen(&settings)) {
        settings.endGroup();
        return {};
    }
    const QByteArray raw = settings.value(seenKey(root)).toByteArray();
    settings.endGroup();
    return stampsFrom(raw);
}

void writeSeen(const QString &root, const QHash<QString, QString> &seen)
{
    QJsonObject object;
    for (auto it = seen.cbegin(); it != seen.cend(); ++it)
        object.insert(it.key(), it.value());
    QSettings settings;
    settings.beginGroup(QStringLiteral("seenFiles"));
    settings.setValue(QStringLiteral("version"), seenVersion);
    settings.setValue(seenKey(root), QJsonDocument(object).toJson(QJsonDocument::Compact));
    settings.endGroup();
}

bool staleFile(const FileDiff &file, const QHash<QString, QString> &seen)
{
    const QString prior = seen.value(file.path());
    return !prior.isEmpty() && prior != fileStamp(file);
}

QColor staleBg(const QWidget *tree)
{
    const bool dark = tree->palette().color(QPalette::Base).lightness() < 128;
    return dark ? QColor(92, 70, 32) : QColor(255, 236, 196);
}

class SeenRow : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const QBrush brush = index.data(Qt::BackgroundRole).value<QBrush>();
        if (brush.style() == Qt::NoBrush || option.state.testFlag(QStyle::State_Selected)) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
        paintStale(painter, option, index, brush);
    }

private:
    void paintStale(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index,
                    const QBrush &brush) const
    {
        painter->fillRect(option.rect, brush);
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        const QRect textRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        const QVariant ink = index.data(Qt::ForegroundRole);
        painter->setPen(ink.canConvert<QBrush>() ? ink.value<QBrush>().color() : opt.palette.color(QPalette::Text));
        painter->setFont(opt.font);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                          opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, textRect.width()));
    }
};

void installSeenRows(QTreeWidget *tree)
{
    tree->setItemDelegate(new SeenRow(tree));
}

void clearSeenMark(QTreeWidget *tree, int file)
{
    for (QTreeWidgetItem *item : tree->findItems(QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive)) {
        if (item->data(0, Qt::UserRole).toInt() != file)
            continue;
        item->setBackground(0, QBrush());
        item->setToolTip(0, item->toolTip(0).section(QLatin1Char('\n'), 0, 0));
        return;
    }
}
