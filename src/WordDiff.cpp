// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDetail.hpp"

#include <QVector>

namespace {

struct Tok {
    int start = 0;
    int end = 0;
    QString key;
};

bool wordChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

int tokenEnd(const QString &line, int index, bool word)
{
    int end = index + 1;
    while (end < line.size() && wordChar(line.at(end)) == word)
        ++end;
    return end;
}

QString tokenKey(const QString &line, int start, int end, bool word)
{
    const QString text = line.mid(start, end - start);
    return word ? text.toLower() : text;
}

QList<Tok> tokenize(const QString &line)
{
    QList<Tok> tokens;
    int index = 0;
    while (index < line.size()) {
        const bool word = wordChar(line.at(index));
        const int end = tokenEnd(line, index, word);
        Tok token;
        token.start = index;
        token.end = end;
        token.key = tokenKey(line, index, end, word);
        tokens.push_back(token);
        index = end;
    }
    return tokens;
}

void appendSpan(QList<WordSpan> *spans, int start, int end, bool changed)
{
    if (end <= start)
        return;
    if (!spans->isEmpty() && spans->last().changed == changed && spans->last().end == start)
        spans->last().end = end;
    else
        spans->push_back(WordSpan{start, end, changed});
}

int &cellAt(QVector<int> &table, int cols, int row, int col)
{
    return table[row * (cols + 1) + col];
}

const int &cellAt(const QVector<int> &table, int cols, int row, int col)
{
    return table[row * (cols + 1) + col];
}

void fillCell(QVector<int> &table, const QList<Tok> &left, const QList<Tok> &right, int row, int col)
{
    const int cols = right.size();
    if (left.at(row - 1).key == right.at(col - 1).key)
        cellAt(table, cols, row, col) = cellAt(table, cols, row - 1, col - 1) + 1;
    else
        cellAt(table, cols, row, col) = qMax(cellAt(table, cols, row - 1, col), cellAt(table, cols, row, col - 1));
}

QVector<int> lcsTable(const QList<Tok> &left, const QList<Tok> &right)
{
    const int rows = left.size();
    const int cols = right.size();
    QVector<int> table((rows + 1) * (cols + 1), 0);
    for (int row = 1; row <= rows; ++row) {
        for (int col = 1; col <= cols; ++col)
            fillCell(table, left, right, row, col);
    }
    return table;
}

void walkLcs(const QVector<int> &table, const QList<Tok> &left, const QList<Tok> &right,
             QVector<char> &leftMatch, QVector<char> &rightMatch)
{
    const int cols = right.size();
    int row = left.size();
    int col = cols;
    while (row > 0 && col > 0) {
        if (left.at(row - 1).key == right.at(col - 1).key) {
            leftMatch[row - 1] = 1;
            rightMatch[col - 1] = 1;
            --row;
            --col;
        } else if (cellAt(table, cols, row - 1, col) >= cellAt(table, cols, row, col - 1)) {
            --row;
        } else {
            --col;
        }
    }
}

void spansFor(const QList<Tok> &tokens, const QVector<char> &match, QList<WordSpan> *out)
{
    for (int index = 0; index < tokens.size(); ++index)
        appendSpan(out, tokens.at(index).start, tokens.at(index).end, !match.at(index));
}

void markWhole(const QString &text, QList<WordSpan> *spans)
{
    appendSpan(spans, 0, text.size(), true);
}

bool tooLong(const QList<Tok> &left, const QList<Tok> &right)
{
    return left.size() > 400 || right.size() > 400;
}

bool hasLetter(const QString &key)
{
    for (const QChar &ch : key) {
        if (ch.isLetterOrNumber())
            return true;
    }
    return false;
}

// Spaces and punctuation alone are not a shared word.
bool sharesWord(const QList<Tok> &tokens, const QVector<char> &match)
{
    for (int index = 0; index < tokens.size(); ++index) {
        if (match.at(index) && hasLetter(tokens.at(index).key))
            return true;
    }
    return false;
}

} // namespace

void wordDiff(const QString &left, const QString &right, QList<WordSpan> *leftSpans, QList<WordSpan> *rightSpans)
{
    if (left == right)
        return;
    const QList<Tok> leftTokens = tokenize(left);
    const QList<Tok> rightTokens = tokenize(right);
    if (leftTokens.isEmpty() && rightTokens.isEmpty())
        return;
    if (tooLong(leftTokens, rightTokens)) {
        markWhole(left, leftSpans);
        markWhole(right, rightSpans);
        return;
    }
    const QVector<int> table = lcsTable(leftTokens, rightTokens);
    QVector<char> leftMatch(leftTokens.size(), 0);
    QVector<char> rightMatch(rightTokens.size(), 0);
    walkLcs(table, leftTokens, rightTokens, leftMatch, rightMatch);
    if (!sharesWord(leftTokens, leftMatch))
        return;
    spansFor(leftTokens, leftMatch, leftSpans);
    spansFor(rightTokens, rightMatch, rightSpans);
}
