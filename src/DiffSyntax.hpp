#pragma once

#include "DiffDoc.hpp"

class QColor;
class QFont;
class QPainter;

QList<Piece> sidePieces(const QString &text, const QList<WordSpan> &words, const QList<SynSpan> &syn, const QFont &font,
                        int width);
void paintCode(QPainter &painter, int x, int top, int lineH, const QString &text, const QList<Piece> &pieces,
               const QColor &plain, const QColor &wordBg);
