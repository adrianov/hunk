// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QColor>
#include <QHash>
#include <QRect>
#include <QSet>

class QPainter;
class QTreeWidget;
class QWidget;

void paintReviewCheck(QPainter *painter, const QRect &rect, bool hot, bool selected, const QColor &selectedInk);

QString changeStamp(const FileDiff &file);
QHash<QString, QString> readSeen(const QString &root);
void writeSeen(const QString &root, const QHash<QString, QString> &seen);
QHash<QString, QString> readHidden(const QString &root);
void writeHidden(const QString &root, const QHash<QString, QString> &hidden);
bool staleFile(const FileDiff &file, const QHash<QString, QString> &seen);
bool fileHidden(const FileDiff &file, const QHash<QString, QString> &hidden);
QSet<int> hiddenIndexes(const DiffDoc &doc, const QHash<QString, QString> &hidden);
QRect fileCheckRect(const QRect &row);
void applySeen(DiffDoc *doc, const QHash<QString, QString> &seen);
QColor staleBg(const QWidget *tree);
void clearSeenMark(QTreeWidget *tree, int file);
void installSeenRows(QTreeWidget *tree);
