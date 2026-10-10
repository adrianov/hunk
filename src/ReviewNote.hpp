// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

// One review comment anchored to a file line, or a span when end is past line.
struct ReviewNote {
    QString path;
    bool oldSide = false;
    int line = 0;
    int end = 0;
    QString snippet;
    QString body;
    bool inDiff = true;
};

inline QString noteSpan(const ReviewNote &note)
{
    QString span = QString::number(note.line);
    if (note.end > note.line)
        span += QLatin1Char('-') + QString::number(note.end);
    return span;
}

inline QString noteKey(const QString &path, bool oldSide, int line)
{
    return path + QLatin1Char('\n') + (oldSide ? QLatin1Char('o') : QLatin1Char('n'))
        + QLatin1Char('\n') + QString::number(line);
}
