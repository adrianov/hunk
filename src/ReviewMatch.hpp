// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QString>

// Path and side checks shared by single-line and span review matching.

inline bool sameFile(const FileDiff &file, const QString &path)
{
    return file.path() == path || file.oldPath == path || file.newPath == path;
}

inline QString sideText(const DiffRow &row, bool oldSide)
{
    return oldSide ? row.leftText : row.rightText;
}
