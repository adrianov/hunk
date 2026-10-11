// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffDetail.hpp"

#include <QStringList>

namespace {

int lineMargin(const QString &line)
{
    if (line.trimmed().isEmpty())
        return -1;
    int spaces = 0;
    while (spaces < line.size() && line.at(spaces) == QLatin1Char(' '))
        ++spaces;
    return spaces;
}

int commonMargin(const QStringList &lines)
{
    int margin = -1;
    for (const QString &line : lines) {
        const int spaces = lineMargin(line);
        if (spaces < 0)
            continue;
        margin = margin < 0 ? spaces : qMin(margin, spaces);
    }
    return qMax(margin, 0);
}

} // namespace

QString dedent(const QString &text)
{
    QStringList lines;
    for (const QString &line : text.split(QLatin1Char('\n')))
        lines.append(expandTabs(line));
    const int margin = commonMargin(lines);
    if (margin > 0) {
        for (QString &line : lines) {
            if (lineMargin(line) >= margin)
                line.remove(0, margin);
        }
    }
    return lines.join(QLatin1Char('\n'));
}
