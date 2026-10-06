#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "DiffParse.hpp"
#include "MainDetail.hpp"
#include "ReviewStore.hpp"

#include <QDockWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>

namespace {

QString notePreview(const QString &body)
{
    const QString preview = body.trimmed().split(QLatin1Char('\n')).value(0);
    if (preview.isEmpty())
        return {};
    if (preview.size() <= 72)
        return QStringLiteral("  —  ") + preview;
    return QStringLiteral("  —  ") + preview.left(69) + QStringLiteral("…");
}

void addNoteItem(QListWidget *list, const ReviewNote &note)
{
    auto *item = new QListWidgetItem(noteLabel(note));
    item->setForeground(note.inDiff ? QColor(QStringLiteral("#d4d4d4")) : QColor(QStringLiteral("#858585")));
    item->setToolTip(note.body);
    list->addItem(item);
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

} // namespace

QString noteLabel(const ReviewNote &note)
{
    QString label = note.path + QLatin1Char(':') + QString::number(note.line);
    if (note.oldSide)
        label += QStringLiteral(" (old)");
    if (!note.inDiff)
        label += QStringLiteral("  (not in diff)");
    return label + notePreview(note.body);
}

void MainWindow::showNote(int row)
{
    const bool on = row >= 0 && row < m_store->notes().size();
    m_editor->setEnabled(on);
    m_delete->setEnabled(on);
    if (!on || m_noteLock)
        return;
    loadNote(row);
}

void MainWindow::loadNote(int row)
{
    const ReviewNote &note = m_store->notes().at(row);
    if (m_editor->toPlainText() != note.body) {
        m_noteLock = true;
        m_editor->setPlainText(note.body);
        m_noteLock = false;
    }
    const LineHit hit = findLine(m_doc, note.path, note.oldSide, note.line);
    if (hit.file >= 0)
        m_diff->showRow(hit.file, hit.row, note.oldSide);
}

void MainWindow::saveNote()
{
    if (m_noteLock)
        return;
    m_store->setBody(m_notes->currentRow(), m_editor->toPlainText());
}

void MainWindow::refreshNotes()
{
    m_noteLock = true;
    const int row = m_notes->currentRow();
    m_notes->clear();
    for (const ReviewNote &note : m_store->notes())
        addNoteItem(m_notes, note);
    if (row >= 0 && row < m_notes->count())
        m_notes->setCurrentRow(row);
    else {
        m_editor->clear();
        m_editor->setEnabled(false);
        m_delete->setEnabled(false);
    }
    m_noteLock = false;
    const int comments = m_store->notes().size();
    m_dock->setWindowTitle(comments == 0 ? QStringLiteral("Reviews") : QStringLiteral("Reviews (%1)").arg(comments));
}

void MainWindow::updateStatus()
{
    int adds = 0;
    int dels = 0;
    for (const FileDiff &file : m_doc.files) {
        adds += file.adds;
        dels += file.dels;
    }
    statusBar()->showMessage(QStringLiteral("%1 files    +%2  −%3    %4 comments")
                                 .arg(m_doc.files.size())
                                 .arg(adds)
                                 .arg(dels)
                                 .arg(filledNotes(m_store->notes())));
}

void MainWindow::pushNoteKeys()
{
    QSet<QString> keys;
    for (const ReviewNote &note : m_store->notes()) {
        if (note.inDiff)
            keys.insert(noteKey(note.path, note.oldSide, note.line));
    }
    m_diff->setNoteKeys(keys);
}
