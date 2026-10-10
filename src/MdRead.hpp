// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QColor>
#include <QString>

class QTextBrowser;
class QWidget;

bool markdownFile(const FileDiff &file);

struct ReadText {
    QString dir;
    QString left;
    QString right;
    bool pair = false;
    bool oldOnly = false;
};

ReadText readFile(const FileDiff &file, const QString &root);
void fillBrowser(QTextBrowser *view, const QString &text, const QString &dir);
void tintWidget(QWidget *widget, const QColor &color);
void paintGround(QWidget *widget);
