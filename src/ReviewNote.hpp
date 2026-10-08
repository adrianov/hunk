// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

// One review comment anchored to a file line.
struct ReviewNote {
    QString path;
    bool oldSide = false;
    int line = 0;
    QString snippet;
    QString body;
    bool inDiff = true;
};

inline QString noteKey(const QString &path, bool oldSide, int line)
{
    return path + QLatin1Char('\n') + (oldSide ? QLatin1Char('o') : QLatin1Char('n'))
        + QLatin1Char('\n') + QString::number(line);
}
