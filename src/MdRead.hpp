// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "MdHtml.hpp"

#include <QColor>
#include <QString>

class QWebEngineView;
class QWidget;

bool markdownFile(const FileDiff &file);

struct ReadText {
    QString dir;
    MdSide left;
    MdSide right;
    bool pair = false;
    bool oldOnly = false;
};

ReadText readFile(const FileDiff &file, const QString &root);
void fillBrowser(QWebEngineView *view, const MdSide &side, const QString &dir);
void tintWidget(QWidget *widget, const QColor &color);
void paintGround(QWidget *widget);
