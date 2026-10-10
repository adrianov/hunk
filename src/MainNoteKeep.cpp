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

bool listedHere(const QListWidgetItem *item, const QString &path, bool oldSide, int line, int end)
{
    if (item->data(Qt::UserRole).toString() != path || item->data(Qt::UserRole + 1).toBool() != oldSide)
        return false;
    return item->data(Qt::UserRole + 2).toInt() == line && item->data(Qt::UserRole + 3).toInt() == end;
}

int rowFor(const QListWidget *list, const QString &path, bool oldSide, int line, int end)
{
    if (line <= 0)
        return -1;
    for (int index = 0; index < list->count(); ++index) {
        if (listedHere(list->item(index), path, oldSide, line, end))
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

int MainWindow::keptNote(int slot, int oldCount, const QString &path, bool oldSide, int line, int end,
                         const QString &body, bool editing)
{
    const int exact = rowFor(m_notes, path, oldSide, line, end);
    if (exact >= 0 || !editing)
        return exact;
    const int shifted = shiftedSlot(slot, oldCount, path, oldSide, m_store->notes());
    return shifted >= 0 ? shifted : bodyRow(path, oldSide, m_store->notes(), body);
}
