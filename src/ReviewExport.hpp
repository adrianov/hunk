// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "ReviewNote.hpp"

#include <QList>

// Markdown an agent can apply: each comment is `path:line` or `path:first-last`, plus the line text.
QString reviewMarkdown(const QString &title, const QList<ReviewNote> &notes);
