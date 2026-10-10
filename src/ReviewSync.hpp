// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"
#include "ReviewNote.hpp"

#include <QList>
#include <QString>

// Keep each comment on its line after the diff changes. Returns whether a note moved or was removed.
bool applyNotes(QList<ReviewNote> *notes, const DiffDoc &doc, const QString &root, bool dropChanged);
