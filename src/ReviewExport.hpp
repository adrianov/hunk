#pragma once

#include "ReviewNote.hpp"

#include <QList>

// Markdown an agent can apply: each comment is `path:line` plus the line text.
QString reviewMarkdown(const QString &title, const QList<ReviewNote> &notes);
