// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainSeen.hpp"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTreeWidget>

namespace {

constexpr int seenVersion = 3;

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

} // namespace

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
    return !prior.isEmpty() && prior != changeStamp(file);
}

QColor staleBg(const QWidget *tree)
{
    const bool dark = tree->palette().color(QPalette::Base).lightness() < 128;
    return dark ? QColor(72, 58, 32) : QColor(255, 246, 220);
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
