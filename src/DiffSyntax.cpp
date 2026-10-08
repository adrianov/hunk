// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffSyntax.hpp"

#include "DiffColors.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>

#include <algorithm>

namespace {

void paintPiece(QPainter &painter, int *x, int baseline, int top, int height, const QString &fragment,
                const QColor &color, bool changed, const QColor &wordBg)
{
    const int width = QFontMetrics(painter.font()).horizontalAdvance(fragment);
    if (changed && wordBg.isValid())
        painter.fillRect(*x, top, width, height, wordBg);
    painter.setPen(color);
    painter.drawText(*x, baseline, fragment);
    *x += width;
}

int fitEnd(const QFontMetrics &metrics, const QString &text, int start, int width)
{
    int low = start;
    int high = int(text.size());
    while (low < high) {
        const int mid = (low + high + 1) / 2;
        if (metrics.horizontalAdvance(QString::fromRawData(text.constData() + start, mid - start)) <= width)
            low = mid;
        else
            high = mid - 1;
    }
    return low;
}

int wordBreak(const QString &text, int start, int end)
{
    if (end >= text.size())
        return end;
    int at = end;
    while (at > start && !text.at(at - 1).isSpace())
        --at;
    return at > start ? at : end;
}

void addEnds(QList<int> *bounds, int start, int end)
{
    bounds->append(start);
    bounds->append(end);
}

QList<int> pieceBounds(const QString &text, const QList<WordSpan> &words, const QList<SynSpan> &syn,
                       const QList<WrapSpan> &wraps)
{
    QList<int> bounds;
    addEnds(&bounds, 0, int(text.size()));
    for (const SynSpan &span : syn)
        addEnds(&bounds, span.start, span.end);
    for (const WordSpan &span : words)
        addEnds(&bounds, span.start, span.end);
    for (const WrapSpan &span : wraps)
        addEnds(&bounds, span.start, span.end);
    std::sort(bounds.begin(), bounds.end());
    bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
    return bounds;
}

SynKind synKind(const QList<SynSpan> &syn, int index, int *cursor)
{
    while (*cursor < syn.size() && syn.at(*cursor).end <= index)
        ++*cursor;
    if (*cursor < syn.size() && syn.at(*cursor).start <= index)
        return syn.at(*cursor).kind;
    return SynKind::Plain;
}

bool wordChanged(const QList<WordSpan> &words, int index, int *cursor)
{
    while (*cursor < words.size() && words.at(*cursor).end <= index)
        ++*cursor;
    if (*cursor < words.size() && words.at(*cursor).start <= index)
        return words.at(*cursor).changed;
    return false;
}

bool atWrap(const QList<WrapSpan> &wraps, int index, int *cursor)
{
    while (*cursor < wraps.size() && wraps.at(*cursor).start < index)
        ++*cursor;
    return *cursor < wraps.size() && wraps.at(*cursor).start == index;
}

QList<WrapSpan> wrapLine(const QString &text, const QFont &font, int width)
{
    QList<WrapSpan> slices;
    const QFontMetrics metrics(font);
    const int limit = width > 0 ? width : 1;
    int start = 0;
    while (start < text.size()) {
        int end = fitEnd(metrics, text, start, limit);
        if (end <= start)
            end = start + 1;
        else
            end = wordBreak(text, start, end);
        slices.append({start, end});
        start = end;
    }
    if (slices.isEmpty())
        slices.append({0, 0});
    return slices;
}

QList<Piece> pieceList(const QString &text, const QList<WordSpan> &words, const QList<SynSpan> &syn,
                       const QList<WrapSpan> &wraps)
{
    const QList<int> bounds = pieceBounds(text, words, syn, wraps);
    QList<Piece> pieces;
    struct Walk {
        int syn = 0;
        int word = 0;
        int wrap = 0;
    } walk;
    for (int index = 1; index < bounds.size(); ++index) {
        const int start = bounds.at(index - 1);
        const int end = bounds.at(index);
        if (end <= start)
            continue;
        pieces.append(Piece{start, end, synKind(syn, start, &walk.syn), wordChanged(words, start, &walk.word),
                            atWrap(wraps, start, &walk.wrap)});
    }
    if (pieces.isEmpty())
        pieces.append(Piece{0, 0, SynKind::Plain, false, true});
    return pieces;
}

} // namespace

QList<Piece> sidePieces(const QString &text, const QList<WordSpan> &words, const QList<SynSpan> &syn, const QFont &font,
                        int width)
{
    return pieceList(text, words, syn, wrapLine(text, font, width));
}

namespace {

QFont pieceFont(const QFont &base, SynKind kind)
{
    QFont font = base;
    font.setBold(kind == SynKind::Strong);
    font.setItalic(kind == SynKind::Emph);
    font.setStrikeOut(kind == SynKind::Strike);
    return font;
}

QColor pieceColor(SynKind kind, const QColor &plain)
{
    if (kind == SynKind::Plain || kind == SynKind::Strong || kind == SynKind::Emph || kind == SynKind::Strike)
        return plain;
    return synColor(kind);
}

} // namespace

void paintCode(QPainter &painter, int x, int top, int lineH, const QString &text, const QList<Piece> &pieces,
               const QColor &plain, const QColor &wordBg)
{
    const QFont base = painter.font();
    const QFontMetrics metrics(base);
    int baseline = top;
    int drawX = x;
    bool lined = false;
    for (const Piece &piece : pieces) {
        if (piece.newLine) {
            if (lined)
                top += lineH;
            lined = true;
            drawX = x;
            baseline = top + (lineH - metrics.height()) / 2 + metrics.ascent();
        }
        if (piece.end <= piece.start)
            continue;
        painter.setFont(pieceFont(base, piece.kind));
        paintPiece(painter, &drawX, baseline, top, lineH, text.mid(piece.start, piece.end - piece.start),
                   pieceColor(piece.kind, plain), piece.changed, wordBg);
    }
}
