#include "ReviewExport.hpp"

QString trimSnippet(QString snippet)
{
    snippet.replace(QLatin1Char('\n'), QLatin1Char(' '));
    while (snippet.endsWith(QLatin1Char(' ')) || snippet.endsWith(QLatin1Char('\t')))
        snippet.chop(1);
    return snippet;
}

QString noteHeading(const ReviewNote &note)
{
    QString heading = QStringLiteral("\n## `") + note.path + QLatin1Char(':') + QString::number(note.line) + QLatin1Char('`');
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
    const QString snippet = trimSnippet(note.snippet);
    if (!snippet.isEmpty())
        block += QStringLiteral("> ") + snippet + QLatin1Char('\n');
    return block + QLatin1Char('\n') + text + QLatin1Char('\n');
}

QString reviewMarkdown(const QString &title, const QList<ReviewNote> &notes)
{
    QString body;
    int count = 0;
    for (const ReviewNote &note : notes) {
        const QString block = noteBlock(note);
        if (block.isEmpty())
            continue;
        body += block;
        ++count;
    }
    if (count == 0)
        return {};
    return QStringLiteral("# Review: ") + title
        + QStringLiteral("\n\nFile references are `path:line` on the new side. `(old)` means the line number before the change.\n")
        + body;
}
