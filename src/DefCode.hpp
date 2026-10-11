// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QWidget>

// Colored, wrapped definition text. Uses the same spans and line breaks as the diff.
class DefCode : public QWidget {
public:
    explicit DefCode(QWidget *parent = nullptr);
    QSize showText(const QString &text, const QString &path, const QFont &font, int barW);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Line {
        QString text;
        QList<Piece> pieces;
        int rows = 1;
    };

    int showLines(const FileDiff &file, const QFont &font, int textW);

    QFont m_font;
    int m_lineH = 1;
    QList<Line> m_lines;
};
