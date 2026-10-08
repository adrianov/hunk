#include "MainSeen.hpp"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeWidget>

namespace {

QString seenKey(const QString &root)
{
    return QString::fromLatin1(QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Sha1).toHex());
}

void hashRow(QCryptographicHash *hash, const DiffRow &row)
{
    if (row.kind == RowKind::Context)
        return;
    hash->addData(QByteArray::number(static_cast<int>(row.kind)));
    hash->addData(row.leftText.toUtf8());
    hash->addData(row.rightText.toUtf8());
}

} // namespace

QString fileStamp(const FileDiff &file)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(file.path().toUtf8());
    hash.addData(QByteArray::number(file.adds));
    hash.addData(QByteArray::number(file.dels));
    hash.addData(file.binary ? "1" : "0");
    for (const DiffRow &row : file.rows)
        hashRow(&hash, row);
    return QString::fromLatin1(hash.result().toHex());
}

QHash<QString, QString> readSeen(const QString &root)
{
    QHash<QString, QString> seen;
    if (root.isEmpty())
        return seen;
    QSettings settings;
    settings.beginGroup(QStringLiteral("seenFiles"));
    const QJsonObject object = QJsonDocument::fromJson(settings.value(seenKey(root)).toByteArray()).object();
    settings.endGroup();
    for (auto it = object.begin(); it != object.end(); ++it)
        seen.insert(it.key(), it.value().toString());
    return seen;
}

void writeSeen(const QString &root, const QHash<QString, QString> &seen)
{
    QJsonObject object;
    for (auto it = seen.cbegin(); it != seen.cend(); ++it)
        object.insert(it.key(), it.value());
    QSettings settings;
    settings.beginGroup(QStringLiteral("seenFiles"));
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
