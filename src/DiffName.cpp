// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "LangTune.hpp"

namespace {

bool receiverChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('@');
}

int constHead(const QString &text, int at)
{
    while (at >= 2 && text.at(at - 1) == QLatin1Char(':') && text.at(at - 2) == QLatin1Char(':')) {
        at -= 2;
        while (at > 0 && receiverChar(text.at(at - 1)))
            --at;
    }
    return at;
}

QString receiverAt(const QString &text, int start)
{
    int tail = start - 1;
    if (tail > 0 && text.at(tail - 1) == QLatin1Char('&'))
        --tail;
    int at = tail;
    while (at > 0 && receiverChar(text.at(at - 1)))
        --at;
    at = constHead(text, at);
    return at == tail ? QString() : text.mid(at, tail - at);
}

QString dotReceiver(const QString &text, const QList<SynSpan> &spans, int column)
{
    for (const SynSpan &span : spans) {
        if (column < span.start || column >= span.end || span.start < 1 || text.at(span.start - 1) != QLatin1Char('.'))
            continue;
        return receiverAt(text, span.start);
    }
    return {};
}

QString spanName(const QString &text, const QList<SynSpan> &spans, int column, const QString &path)
{
    const LangTune *tune = langTune(langFor(path));
    if (!tune)
        return {};
    for (const SynSpan &span : spans) {
        if (column < span.start || column >= span.end || !tuneKind(tune, span.kind))
            continue;
        return tune->name ? tune->name(text, span.start, span.end, span.kind) : text.mid(span.start, span.end - span.start);
    }
    return {};
}

} // namespace

QString DiffCanvas::methodName(int file, int row, bool old, int column, int *line, QString *receiver) const
{
    const FileDiff &fileDiff = m_doc.files.at(file);
    const DiffRow &lineRow = fileDiff.rows.at(row);
    const QString path = sidePath(fileDiff, old);
    const QString text = old ? lineRow.leftText : lineRow.rightText;
    const QList<SynSpan> spans = old ? lineRow.leftSyn : lineRow.rightSyn;
    *line = old ? lineRow.leftNum : lineRow.rightNum;
    if (receiver)
        *receiver = dotReceiver(text, spans, column);
    return spanName(text, spans, column, path);
}
