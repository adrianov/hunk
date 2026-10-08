// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "ReviewStore.hpp"

#include <QListWidget>

namespace {

bool sameNote(const ReviewNote &note, const QString &path, bool oldSide)
{
    return note.path == path && note.oldSide == oldSide;
}

int rowFor(const QListWidget *list, const QString &path, bool oldSide, int line)
{
    if (line <= 0)
        return -1;
    for (int index = 0; index < list->count(); ++index) {
        const QListWidgetItem *item = list->item(index);
        if (item->data(Qt::UserRole).toString() == path && item->data(Qt::UserRole + 1).toBool() == oldSide
            && item->data(Qt::UserRole + 2).toInt() == line)
            return index;
    }
    return -1;
}

int shiftedSlot(int slot, int oldCount, const QString &path, bool oldSide, const QList<ReviewNote> &notes)
{
    if (oldCount != notes.size() || slot < 0 || slot >= notes.size() || !sameNote(notes.at(slot), path, oldSide))
        return -1;
    return slot;
}

int bodyRow(const QString &path, bool oldSide, const QList<ReviewNote> &notes, const QString &body)
{
    int found = -1;
    for (int index = 0; index < notes.size(); ++index) {
        const ReviewNote &note = notes.at(index);
        if (!sameNote(note, path, oldSide) || note.body != body)
            continue;
        if (found >= 0)
            return -1;
        found = index;
    }
    return found;
}

} // namespace

int MainWindow::keptNote(int slot, int oldCount, const QString &path, bool oldSide, int line, const QString &body,
                         bool editing)
{
    const int exact = rowFor(m_notes, path, oldSide, line);
    if (exact >= 0 || !editing)
        return exact;
    const int shifted = shiftedSlot(slot, oldCount, path, oldSide, m_store->notes());
    return shifted >= 0 ? shifted : bodyRow(path, oldSide, m_store->notes(), body);
}
