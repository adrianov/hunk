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
constexpr int hiddenVersion = 1;

QString seenKey(const QString &root)
{
    return QString::fromLatin1(QCryptographicHash::hash(root.toUtf8(), QCryptographicHash::Sha1).toHex());
}

bool keepGroup(QSettings *settings, int version)
{
    if (settings->value(QStringLiteral("version")).toInt() == version)
        return true;
    settings->remove(QString());
    settings->setValue(QStringLiteral("version"), version);
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

QHash<QString, QString> readGroup(const QString &root, const QString &group, int version)
{
    if (root.isEmpty())
        return {};
    QSettings settings;
    settings.beginGroup(group);
    if (!keepGroup(&settings, version)) {
        settings.endGroup();
        return {};
    }
    const QByteArray raw = settings.value(seenKey(root)).toByteArray();
    settings.endGroup();
    return stampsFrom(raw);
}

void writeGroup(const QString &root, const QString &group, int version, const QHash<QString, QString> &stamps)
{
    QJsonObject object;
    for (auto it = stamps.cbegin(); it != stamps.cend(); ++it)
        object.insert(it.key(), it.value());
    QSettings settings;
    settings.beginGroup(group);
    settings.setValue(QStringLiteral("version"), version);
    settings.setValue(seenKey(root), QJsonDocument(object).toJson(QJsonDocument::Compact));
    settings.endGroup();
}

} // namespace

QHash<QString, QString> readSeen(const QString &root)
{
    return readGroup(root, QStringLiteral("seenFiles"), seenVersion);
}

void writeSeen(const QString &root, const QHash<QString, QString> &seen)
{
    writeGroup(root, QStringLiteral("seenFiles"), seenVersion, seen);
}

QHash<QString, QString> readHidden(const QString &root)
{
    return readGroup(root, QStringLiteral("hiddenFiles"), hiddenVersion);
}

void writeHidden(const QString &root, const QHash<QString, QString> &hidden)
{
    writeGroup(root, QStringLiteral("hiddenFiles"), hiddenVersion, hidden);
}

bool staleFile(const FileDiff &file, const QHash<QString, QString> &seen)
{
    const QString prior = seen.value(file.path());
    return !prior.isEmpty() && prior != changeStamp(file);
}

bool fileHidden(const FileDiff &file, const QHash<QString, QString> &hidden)
{
    const QString prior = hidden.value(file.path());
    return !prior.isEmpty() && prior == changeStamp(file);
}

QSet<int> hiddenIndexes(const DiffDoc &doc, const QHash<QString, QString> &hidden)
{
    QSet<int> skip;
    for (int index = 0; index < doc.files.size(); ++index) {
        if (fileHidden(doc.files.at(index), hidden))
            skip.insert(index);
    }
    return skip;
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
