// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

namespace {

void collectPaths(const QSet<QString> &from, const QSet<QString> &other, QSet<QString> *paths)
{
    for (const QString &key : from) {
        if (!other.contains(key))
            paths->insert(key.section(QLatin1Char('\n'), 0, 0));
    }
}

} // namespace

void DiffCanvas::ensureRow(int fileIndex, int rowIndex, bool single)
{
    DiffRow &row = m_doc.files[fileIndex].rows[rowIndex];
    if (m_wrapW.value(fileIndex) == m_viewW && (!row.leftPiece.isEmpty() || !row.rightPiece.isEmpty()))
        return;
    wrapRow(&row, single);
    m_wrapW[fileIndex] = m_viewW;
}

void DiffCanvas::applyNotePaths(const QSet<QString> &keys)
{
    QSet<QString> paths;
    collectPaths(m_notes, keys, &paths);
    collectPaths(keys, m_notes, &paths);
    m_notes = keys;
    for (int fileIndex = 0; fileIndex < m_doc.files.size(); ++fileIndex) {
        if (paths.contains(m_doc.files.at(fileIndex).path()))
            relayoutFile(fileIndex);
    }
    publishLayout();
}

void DiffCanvas::setNoteKeys(const QSet<QString> &keys)
{
    if (m_notes == keys)
        return;
    if (m_doc.files.isEmpty()) {
        m_notes = keys;
        viewport()->update();
        return;
    }
    applyNotePaths(keys);
}

void DiffCanvas::dropPieces()
{
    for (FileDiff &file : m_doc.files) {
        for (DiffRow &row : file.rows) {
            row.leftPiece.clear();
            row.rightPiece.clear();
        }
    }
}

// Line pieces depend on each pane's width. Drop them when that width changes.
void DiffCanvas::prepareWidth()
{
    if (!layoutStale())
        return;
    const int width = qMax(1, viewport()->width());
    dropPieces();
    m_viewW = width;
    m_splitW = pairSplit(width);
}

void DiffCanvas::publishLayout()
{
    updateScroll();
    viewport()->update();
    emitVisibleFile();
}

int DiffCanvas::fileBands(int fileIndex, int *end) const
{
    int start = 0;
    while (start < m_bands.size() && m_bands.at(start).file != fileIndex)
        ++start;
    int cursor = start;
    while (cursor < m_bands.size() && m_bands.at(cursor).file == fileIndex)
        ++cursor;
    *end = cursor;
    return start;
}

void DiffCanvas::appendTail(const QList<Band> &tail, int delta)
{
    for (const Band &band : tail) {
        Band shifted = band;
        shifted.y += delta;
        m_bands.append(shifted);
    }
    m_docH += delta;
}

void DiffCanvas::relayoutFile(int fileIndex)
{
    if (layoutStale()) {
        rebuild();
        return;
    }
    int end = 0;
    const int start = fileBands(fileIndex, &end);
    if (start >= m_bands.size())
        return;
    const int top = m_bands.at(start).y;
    const int nextY = end < m_bands.size() ? m_bands.at(end).y : m_docH - 12;
    const QList<Band> tail = m_bands.mid(end);
    m_bands.resize(start);
    int y = top;
    addFileBands(fileIndex, &y);
    appendTail(tail, y - nextY);
}
