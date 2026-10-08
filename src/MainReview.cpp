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
#include <QStatusBar>

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

QString copyStatus(int count)
{
    if (count == 1)
        return QStringLiteral("Copied 1 review comment");
    return QStringLiteral("Copied %1 review comments").arg(count);
}

} // namespace

void MainWindow::commentSelection()
{
    if (m_editor->hasFocus() || m_filter->hasFocus() || m_base->hasFocus() || m_head->hasFocus())
        return;
    if (!m_diff->hasSelection()) {
        statusBar()->showMessage(QStringLiteral("Click a line number to comment"));
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
    QString snippet;
    if (!commentLine(file, row, &oldSide, &line, &snippet))
        return;
    m_notes->setCurrentRow(m_store->ensure(m_doc.files.at(file).path(), oldSide, line, snippet));
    m_editor->setFocus();
    m_diff->showRow(file, row, oldSide);
}

void MainWindow::copyReviews()
{
    const QString markdown = reviewMarkdown(m_title.isEmpty() ? QStringLiteral("diff") : m_title, m_store->notes());
    if (markdown.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("No review comments to copy"));
        return;
    }
    QGuiApplication::clipboard()->setText(markdown);
    statusBar()->showMessage(copyStatus(filledNotes(m_store->notes())));
}
