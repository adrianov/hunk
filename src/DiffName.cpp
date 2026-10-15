// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "LangTune.hpp"
#include "RubyDetail.hpp"

namespace {

QString dotReceiver(const QString &text, const QList<SynSpan> &spans, int column)
{
    for (const SynSpan &span : spans) {
        if (column < span.start || column >= span.end || span.start < 1 || text.at(span.start - 1) != QLatin1Char('.'))
            continue;
        return rubyReceiver(text, span.start);
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

QString ledReceiver(const QString &root, const QString &path, int line, int column, const QString &name,
                    const QString &text, const QString &receiver)
{
    const QString lead = rubyLead(defSpot(root, path, line, column, name, receiver), text);
    return lead.isEmpty() ? receiver : lead;
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
    const QString name = spanName(text, spans, column, path);
    if (receiver)
        *receiver = ledReceiver(m_root, path, *line, column, name, text, dotReceiver(text, spans, column));
    return name;
}
