// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "SyntaxRule.hpp"

#include <QDir>
#include <QFileInfo>
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

} // namespace

// The repository file named by the markdown link under the pointer.
QString DiffCanvas::linkPath(const TextMark &hit) const
{
    if (!hit.code || m_root.isEmpty())
        return {};
    const FileDiff &file = m_doc.files.at(hit.file);
    const QString path = sidePath(file, hit.old);
    if (langFor(path) != Lang::Markdown)
        return {};
    const DiffRow &row = file.rows.at(hit.row);
    const QString rel =
        linkRel(path, QUrl(linkTarget(hit.old ? row.leftText : row.rightText, hit.old ? row.leftSyn : row.rightSyn, hit.pos)));
    return QFileInfo(QDir(m_root).filePath(rel)).isFile() ? rel : QString();
}
