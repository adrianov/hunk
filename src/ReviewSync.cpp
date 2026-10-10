// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "ReviewSync.hpp"

#include "DiffParse.hpp"
#include "ReviewShift.hpp"

#include <QStringList>

namespace {

QString lineText(const DiffDoc &doc, const LineHit &hit, bool oldSide)
{
    const DiffRow &row = doc.files.at(hit.file).rows.at(hit.row);
    return oldSide ? row.leftText : row.rightText;
}

QString spanText(const DiffDoc &doc, const ReviewNote &note)
{
    QStringList lines;
    for (int line = note.line; line <= note.end; ++line) {
        const LineHit hit = findLine(doc, note.path, note.oldSide, line);
        if (hit.file < 0)
            return {};
        lines.append(lineText(doc, hit, note.oldSide));
    }
    return lines.join(QLatin1Char('\n'));
}

int movedSpan(const DiffDoc &doc, const ReviewNote &note, const QString &root, LineCache *cache)
{
    ReviewNote head = note;
    head.end = 0;
    head.snippet = note.snippet.section(QLatin1Char('\n'), 0, 0);
    const int start = shiftedLine(doc, root, head, cache);
    if (start <= 0)
        return 0;
    ReviewNote moved = note;
    moved.line = start;
    moved.end = start + (note.end - note.line);
    return spanText(doc, moved) == note.snippet ? start : 0;
}

bool markMissing(ReviewNote *note)
{
    if (!note->inDiff)
        return false;
    note->inDiff = false;
    return true;
}

bool adoptLine(ReviewNote *note, const QString &text)
{
    if (note->inDiff && note->snippet == text)
        return false;
    note->inDiff = true;
    note->snippet = text;
    return true;
}

bool followLine(ReviewNote *note, int line, const DiffDoc &doc)
{
    note->line = line;
    const LineHit hit = findLine(doc, note->path, note->oldSide, line);
    if (hit.file < 0) {
        markMissing(note);
        return true;
    }
    adoptLine(note, lineText(doc, hit, note->oldSide));
    return true;
}

bool dropStale(QList<ReviewNote> *notes, int index, bool inDiff, const QString &text, bool dropChanged)
{
    ReviewNote &note = (*notes)[index];
    if (!inDiff) {
        if (!dropChanged)
            return markMissing(&note);
        notes->removeAt(index);
        return true;
    }
    if (dropChanged && !note.snippet.isEmpty()) {
        notes->removeAt(index);
        return true;
    }
    return adoptLine(&note, text);
}

int syncRange(QList<ReviewNote> *notes, int index, const DiffDoc &doc, const QString &root, LineCache *cache,
               bool dropChanged)
{
    ReviewNote &note = (*notes)[index];
    if (note.end <= note.line)
        return -1;
    const QString text = spanText(doc, note);
    if (!text.isEmpty() && (note.snippet.isEmpty() || text == note.snippet))
        return adoptLine(&note, text) ? 1 : 0;
    const int start = movedSpan(doc, note, root, cache);
    if (start > 0) {
        note.end += start - note.line;
        note.line = start;
        adoptLine(&note, note.snippet);
        return 1;
    }
    return dropStale(notes, index, !text.isEmpty(), text, dropChanged) ? 1 : 0;
}

bool syncNote(QList<ReviewNote> *notes, int index, const DiffDoc &doc, const QString &root, LineCache *cache,
              bool dropChanged)
{
    const int range = syncRange(notes, index, doc, root, cache, dropChanged);
    if (range >= 0)
        return range == 1;
    ReviewNote &note = (*notes)[index];
    const LineHit hit = findLine(doc, note.path, note.oldSide, note.line);
    const QString text = hit.file >= 0 ? lineText(doc, hit, note.oldSide) : QString();
    if (hit.file >= 0 && (note.snippet.isEmpty() || text == note.snippet))
        return adoptLine(&note, text);
    const int moved = shiftedLine(doc, root, note, cache);
    if (moved > 0)
        return followLine(&note, moved, doc);
    return dropStale(notes, index, hit.file >= 0, text, dropChanged);
}

} // namespace

bool applyNotes(QList<ReviewNote> *notes, const DiffDoc &doc, const QString &root, bool dropChanged)
{
    LineCache cache;
    bool changed = false;
    for (int index = notes->size() - 1; index >= 0; --index)
        changed = syncNote(notes, index, doc, root, &cache, dropChanged) || changed;
    return changed;
}
