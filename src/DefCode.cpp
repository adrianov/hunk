// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DefCode.hpp"

#include "DiffColors.hpp"
#include "DiffSyntax.hpp"
#include "Syntax.hpp"

#include <QFontMetrics>
#include <QPainter>

namespace {

FileDiff colorDef(const QString &text, const QString &path)
{
    FileDiff file;
    file.newPath = path;
    for (const QString &line : text.split(QLatin1Char('\n'))) {
        DiffRow row;
        row.rightText = line;
        file.rows.append(row);
    }
    DiffDoc doc;
    doc.files.append(file);
    highlightDoc(&doc);
    return doc.files.at(0);
}

int textWidth(const QFont &font, const QString &code)
{
    const QFontMetrics metrics(font);
    int width = 200;
    for (const QString &line : code.split(QLatin1Char('\n')))
        width = qMax(width, metrics.horizontalAdvance(line));
    return qMin(width, 624);
}

int pieceRows(const QList<Piece> &pieces)
{
    int rows = 0;
    for (const Piece &piece : pieces) {
        if (piece.newLine)
            ++rows;
    }
    return qMax(rows, 1);
}

int wrapWidth(int textW, int rows, int lineH, int barW)
{
    if (rows * lineH > 320 && barW > 0)
        return qMax(80, textW - barW);
    return textW;
}

} // namespace

DefCode::DefCode(QWidget *parent)
    : QWidget(parent)
{
}

int DefCode::showLines(const FileDiff &file, const QFont &font, int textW)
{
    m_font = font;
    m_lineH = QFontMetrics(font).height() + 4;
    m_lines.clear();
    int rows = 0;
    for (const DiffRow &row : file.rows) {
        Line line;
        line.text = row.rightText;
        line.pieces = sidePieces(line.text, {}, row.rightSyn, font, textW);
        line.rows = pieceRows(line.pieces);
        rows += line.rows;
        m_lines.append(line);
    }
    setFixedSize(textW + 16, qMax(rows, 1) * m_lineH);
    update();
    return rows;
}

QSize DefCode::showText(const QString &text, const QString &path, const QFont &font, int barW)
{
    const FileDiff file = colorDef(text, path);
    const int lineH = QFontMetrics(font).height() + 4;
    const int textW = textWidth(font, text);
    const int narrowed = wrapWidth(textW, showLines(file, font, textW), lineH, barW);
    if (narrowed != textW)
        showLines(file, font, narrowed);
    const int paneW = QWidget::width() + (QWidget::height() > 320 ? barW : 0);
    return QSize(paneW, qMin(QWidget::height(), 320));
}

void DefCode::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), kBg);
    painter.setFont(m_font);
    int top = 0;
    for (const Line &line : m_lines) {
        paintCode(painter, 8, top, m_lineH, line.text, line.pieces, kText, QColor());
        top += line.rows * m_lineH;
    }
}
