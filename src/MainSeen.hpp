// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QColor>
#include <QHash>

class QTreeWidget;
class QWidget;

QString changeStamp(const FileDiff &file);
QHash<QString, QString> readSeen(const QString &root);
void writeSeen(const QString &root, const QHash<QString, QString> &seen);
bool staleFile(const FileDiff &file, const QHash<QString, QString> &seen);
void applySeen(DiffDoc *doc, const QHash<QString, QString> &seen);
QColor staleBg(const QWidget *tree);
void clearSeenMark(QTreeWidget *tree, int file);
void installSeenRows(QTreeWidget *tree);
