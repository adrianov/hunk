// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "SyntaxRule.hpp"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

namespace {

// The target of the [label](target) under the column, without its title.
QString linkTarget(const QString &text, const QList<SynSpan> &spans, int column)
{
    for (const SynSpan &span : spans) {
        if (text.mid(span.start - 2, 2) == QLatin1String("](") && text.lastIndexOf(QLatin1Char('['), span.start) <= column
            && column <= span.end)
            return text.mid(span.start, span.end - span.start).section(QLatin1Char(' '), 0, 0);
    }
    return {};
}

// That target as a path from the root, which a leading '/' names, as on GitHub.
// Empty for a URL, an #anchor, or a path outside the root.
QString linkRel(const QString &doc, const QUrl &target)
{
    if (!target.scheme().isEmpty() || target.path().isEmpty())
        return {};
    const QString rel = QUrl::fromLocalFile(QLatin1Char('/') + doc).resolved(target).path(QUrl::FullyDecoded).mid(1);
    return rel.startsWith(QLatin1String("../")) ? QString() : rel;
}

// Targets for the path-like word under the column, such as config/initializers/http.rb in a comment:
// beside the doc first, then from the root unless it starts with "./" or "../".
QStringList pathTargets(const QString &text, int column)
{
    static const QRegularExpression path(QStringLiteral(R"(((?:\.\.?/)*)\.?[\w-]+(?:[./]+[\w-]+)+)"));
    for (auto it = path.globalMatch(text); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        if (column < match.capturedStart() || column >= match.capturedEnd())
            continue;
        const QString word = match.captured();
        return match.capturedLength(1) ? QStringList{word} : QStringList{word, QLatin1Char('/') + word};
    }
    return {};
}

// The first of those targets, read from the doc, that is a file under the root.
QString firstFile(const QString &root, const QString &doc, const QStringList &targets)
{
    for (const QString &target : targets)
        if (const QString rel = linkRel(doc, QUrl(target)); !rel.isEmpty() && QFileInfo(QDir(root).filePath(rel)).isFile())
            return rel;
    return {};
}

} // namespace

// The repository file named by the markdown link or the path under the pointer.
QString DiffCanvas::linkPath(const TextMark &hit) const
{
    if (!hit.code || m_root.isEmpty())
        return {};
    const FileDiff &file = m_doc.files.at(hit.file);
    const QString path = sidePath(file, hit.old);
    const DiffRow &row = file.rows.at(hit.row);
    const QString &text = hit.old ? row.leftText : row.rightText;
    const QString target =
        langFor(path) == Lang::Markdown ? linkTarget(text, hit.old ? row.leftSyn : row.rightSyn, hit.pos) : QString();
    return firstFile(m_root, path, QStringList{target} + pathTargets(text, hit.pos));
}
