// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "ReviewExport.hpp"

#include <QStringList>

QString quoteSnippet(const QString &snippet)
{
    QString block;
    for (const QString &line : snippet.split(QLatin1Char('\n'))) {
        QString text = line;
        while (text.endsWith(QLatin1Char(' ')) || text.endsWith(QLatin1Char('\t')))
            text.chop(1);
        block += QStringLiteral("> ") + text + QLatin1Char('\n');
    }
    return block;
}

QString noteHeading(const ReviewNote &note)
{
    QString heading = QStringLiteral("\n## `") + note.path + QLatin1Char(':') + noteSpan(note) + QLatin1Char('`');
    if (note.oldSide)
        heading += QStringLiteral(" (old)");
    return heading + QLatin1Char('\n');
}

QString noteBlock(const ReviewNote &note)
{
    const QString text = note.body.trimmed();
    if (text.isEmpty())
        return {};
    QString block = noteHeading(note);
    if (!note.snippet.isEmpty())
        block += quoteSnippet(note.snippet);
    return block + QLatin1Char('\n') + text + QLatin1Char('\n');
}

QString reviewLead(bool range, bool old)
{
    QString lead = QStringLiteral("\n\nFile references are `path:line` on the new side.");
    if (range)
        lead += QStringLiteral(" A span of lines is `path:first-last`.");
    if (old)
        lead += QStringLiteral(" `(old)` means the line number before the change.");
    return lead;
}

QString reviewMarkdown(const QString &title, const QList<ReviewNote> &notes)
{
    QString body;
    int count = 0;
    bool old = false;
    bool range = false;
    for (const ReviewNote &note : notes) {
        const QString block = noteBlock(note);
        if (block.isEmpty())
            continue;
        body += block;
        old = old || note.oldSide;
        range = range || note.end > note.line;
        ++count;
    }
    if (count == 0)
        return {};
    return QStringLiteral("# Review: ") + title + reviewLead(range, old) + QLatin1Char('\n') + body;
}
