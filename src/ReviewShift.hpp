#pragma once

#include "DiffDoc.hpp"
#include "ReviewNote.hpp"

// Line number where the commented text now sits, or 0 when it did not move.
int shiftedLine(const DiffDoc &doc, const QString &root, const ReviewNote &note);
