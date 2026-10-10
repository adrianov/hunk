// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "ReviewShift.hpp"

#include "DiffParse.hpp"
#include "ReviewMatch.hpp"

namespace {

void addMatch(QList<int> *lines, int line, const QString &text, const QString &snippet)
{
    if (line > 0 && text == snippet)
        lines->push_back(line);
}

QList<int> diffMatches(const DiffDoc &doc, const ReviewNote &note)
{
    QList<int> lines;
    for (const FileDiff &file : doc.files) {
        if (!sameFile(file, note.path))
            continue;
        for (const DiffRow &row : file.rows)
            addMatch(&lines, note.oldSide ? row.leftNum : row.rightNum, sideText(row, note.oldSide), note.snippet);
    }
    return lines;
}

QList<int> fileMatches(const QString &root, const DiffDoc &doc, const ReviewNote &note, LineCache *cache)
{
    QList<int> lines;
    if (root.isEmpty() || note.oldSide)
        return lines;
    const QStringList text = cachedFile(doc, root, note.path, cache);
    for (int index = 0; index < text.size(); ++index)
        addMatch(&lines, index + 1, text.at(index), note.snippet);
    return lines;
}

bool replacedSnippet(const DiffDoc &doc, const ReviewNote &note)
{
    const LineHit hit = findLine(doc, note.path, note.oldSide, note.line);
    if (hit.file < 0)
        return false;
    const DiffRow &row = doc.files.at(hit.file).rows.at(hit.row);
    return sideText(row, !note.oldSide) == note.snippet && sideText(row, note.oldSide) != note.snippet;
}

int chosenLine(const QList<int> &lines, int anchor)
{
    int found = 0;
    for (int line : lines) {
        if (line == anchor)
            return 0;
        if (found == 0)
            found = line;
        else if (line != found)
            return 0;
    }
    return found;
}

struct NearText {
    bool known = false;
    QString text;
};

NearText textAt(const DiffDoc &doc, const ReviewNote &note, int line)
{
    if (line <= 0)
        return {};
    for (const FileDiff &file : doc.files) {
        if (!sameFile(file, note.path))
            continue;
        for (const DiffRow &row : file.rows) {
            const int number = note.oldSide ? row.leftNum : row.rightNum;
            if (number != line)
                continue;
            return {true, sideText(row, note.oldSide)};
        }
    }
    return {};
}

bool beside(const QStringList &lines, int index, const NearText &before, const NearText &after)
{
    if (!before.known && !after.known)
        return false;
    if (before.known && (index == 0 || lines.at(index - 1) != before.text))
        return false;
    if (after.known && (index + 1 >= lines.size() || lines.at(index + 1) != after.text))
        return false;
    return true;
}

int fittingLine(const QStringList &lines, const ReviewNote &note, const NearText &before, const NearText &after)
{
    int found = 0;
    for (int index = 0; index < lines.size(); ++index) {
        if (index + 1 == note.line || lines.at(index) != note.snippet)
            continue;
        if (!beside(lines, index, before, after))
            continue;
        if (found > 0)
            return 0;
        found = index + 1;
    }
    return found;
}

int contextShift(const DiffDoc &doc, const ReviewNote &note, const QStringList &lines)
{
    const int marked = chosenLine(diffMatches(doc, note), note.line);
    if (marked <= 0 || lines.isEmpty())
        return 0;
    return fittingLine(lines, note, textAt(doc, note, marked - 1), textAt(doc, note, marked + 1));
}

} // namespace

int shiftedLine(const DiffDoc &doc, const QString &root, const ReviewNote &note, LineCache *cache)
{
    if (note.snippet.isEmpty() || replacedSnippet(doc, note))
        return 0;
    const int only = chosenLine(diffMatches(doc, note) + fileMatches(root, doc, note, cache), note.line);
    if (only > 0 || root.isEmpty() || note.oldSide)
        return only;
    return contextShift(doc, note, cachedFile(doc, root, note.path, cache));
}
