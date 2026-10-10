// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "ReviewExport.hpp"
#include "ReviewStore.hpp"

#include <QClipboard>
#include <QComboBox>
#include <QGuiApplication>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QStringList>

namespace {

void sideText(const DiffRow &row, bool oldSide, int *line, QString *snippet)
{
    *line = oldSide ? row.leftNum : row.rightNum;
    *snippet = oldSide ? row.leftText : row.rightText;
}

int filledNotes(const QList<ReviewNote> &notes)
{
    int count = 0;
    for (const ReviewNote &note : notes) {
        if (!note.body.trimmed().isEmpty())
            ++count;
    }
    return count;
}

int sideNum(const DiffRow &row, bool oldSide)
{
    return oldSide ? row.leftNum : row.rightNum;
}

void spanBounds(const FileDiff &file, bool oldSide, int from, int to, int *first, int *last)
{
    *first = 0;
    *last = 0;
    for (int index = qMax(0, from); index <= to && index < file.rows.size(); ++index) {
        const int number = sideNum(file.rows.at(index), oldSide);
        if (number <= 0)
            continue;
        if (*first == 0 || number < *first)
            *first = number;
        if (number > *last)
            *last = number;
    }
}

QString spanBody(const FileDiff &file, bool oldSide, int first, int last)
{
    QStringList lines;
    for (const DiffRow &row : file.rows) {
        const int number = sideNum(row, oldSide);
        if (number < first || number > last)
            continue;
        lines.append(oldSide ? row.leftText : row.rightText);
    }
    if (lines.size() != last - first + 1)
        return {};
    return lines.join(QLatin1Char('\n'));
}

QString spanLines(const FileDiff &file, bool oldSide, int from, int to, int *line, int *end)
{
    int first = 0;
    int last = 0;
    spanBounds(file, oldSide, from, to, &first, &last);
    if (last <= first)
        return {};
    const QString body = spanBody(file, oldSide, first, last);
    if (body.isEmpty())
        return {};
    *line = first;
    *end = last;
    return body;
}

void widenComment(const FileDiff &file, const DiffCanvas *canvas, int fileIndex, int row, bool oldSide, int *line,
                  int *end, QString *snippet)
{
    int from = 0;
    int to = 0;
    if (!canvas->markedRows(fileIndex, row, oldSide, &from, &to))
        return;
    const QString span = spanLines(file, oldSide, from, to, line, end);
    if (!span.isEmpty())
        *snippet = span;
}

QString copyStatus(int count)
{
    if (count == 1)
        return QStringLiteral("Copied 1 review comment");
    return QStringLiteral("Copied %1 review comments").arg(count);
}

} // namespace

void MainWindow::commentSelection()
{
    if (m_editor->hasFocus() || m_filter->hasFocus() || m_search->hasFocus() || m_base->hasFocus() || m_head->hasFocus())
        return;
    if (!m_diff->hasSelection()) {
        showStatus(QStringLiteral("Click a line number to comment"));
        return;
    }
    commentAt(m_diff->selectedFile(), m_diff->selectedRow(), m_diff->selectedOldSide());
}

bool MainWindow::commentLine(int file, int row, bool *oldSide, int *line, QString *snippet) const
{
    if (file < 0 || file >= m_doc.files.size())
        return false;
    const FileDiff &diffFile = m_doc.files.at(file);
    if (row < 0 || row >= diffFile.rows.size() || diffFile.path().isEmpty())
        return false;
    const DiffRow &diffRow = diffFile.rows.at(row);
    sideText(diffRow, *oldSide, line, snippet);
    if (*line <= 0) {
        *oldSide = !*oldSide;
        sideText(diffRow, *oldSide, line, snippet);
    }
    return *line > 0;
}

void MainWindow::commentAt(int file, int row, bool oldSide)
{
    int line = 0;
    int end = 0;
    QString snippet;
    if (!commentLine(file, row, &oldSide, &line, &snippet))
        return;
    widenComment(m_doc.files.at(file), m_diff, file, row, oldSide, &line, &end, &snippet);
    m_notes->setCurrentRow(m_store->ensure(m_doc.files.at(file).path(), oldSide, line, snippet, end));
    m_editor->setFocus();
    m_diff->showRow(file, row, oldSide);
}

void MainWindow::copyReviews()
{
    const QString markdown = reviewMarkdown(m_title.isEmpty() ? QStringLiteral("diff") : m_title, m_store->notes());
    if (markdown.isEmpty()) {
        showStatus(QStringLiteral("No review comments to copy"));
        return;
    }
    QGuiApplication::clipboard()->setText(markdown);
    showStatus(copyStatus(filledNotes(m_store->notes())));
}
