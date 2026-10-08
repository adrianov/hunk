// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

// Parse `git diff -w -W --no-prefix --diff-algorithm=histogram` output.
DiffDoc parseDiff(const QString &raw);

// Find a commented line. `oldSide` uses the pre-change line number.
LineHit findLine(const DiffDoc &doc, const QString &path, bool oldSide, int line);
