// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "DiffColors.hpp"
#include "DiffParse.hpp"
#include "MainDetail.hpp"
#include "ReviewStore.hpp"

#include <QEvent>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QTimer>

namespace {

QString notePreview(const QString &body)
{
    const QString preview = body.trimmed().split(QLatin1Char('\n')).value(0);
    if (preview.isEmpty())
        return {};
    return QStringLiteral("  —  ") + preview;
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

QColor noteInk(const QListWidget *list, bool selected, bool inDiff)
{
    if (selected)
        return list->palette().color(QPalette::HighlightedText);
    if (!inDiff)
        return kMuted;
    return list->palette().color(QPalette::Text);
}

void tintLabel(QLabel *label, const QColor &color)
{
    const QPalette::ColorRole role = label->foregroundRole();
    QPalette palette = label->palette();
    if (palette.color(role) == color)
        return;
    palette.setColor(role, color);
    label->setPalette(palette);
}

void placeRow(QListWidget *list, int index, bool inDiff)
{
    QListWidgetItem *item = list->item(index);
    QWidget *row = list->itemWidget(item);
    const QRect rect = list->visualItemRect(item);
    if (!row || rect.width() < 1)
        return;
    if (row->geometry() != rect)
        row->setGeometry(rect);
    if (QLabel *label = rowLabel(row))
        tintLabel(label, noteInk(list, list->hasFocus() && item == list->currentItem(), inDiff));
}

} // namespace

void MainWindow::placeNoteRows()
{
    if (!m_notes)
        return;
    const QList<ReviewNote> &notes = m_store->notes();
    for (int index = 0; index < notes.size() && index < m_notes->count(); ++index)
        placeRow(m_notes, index, notes.at(index).inDiff);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_repoLabel && event->type() == QEvent::Resize)
        showRepoPath();
    return noteEvents(object, event);
}

bool MainWindow::noteEvents(QObject *object, QEvent *event)
{
    if (!m_notes)
        return QMainWindow::eventFilter(object, event);
    const QEvent::Type type = event->type();
    const bool relayout = object == m_notes->viewport() && type == QEvent::Resize;
    const bool refocus = object == m_notes && (type == QEvent::FocusIn || type == QEvent::FocusOut);
    if (relayout || refocus)
        QTimer::singleShot(0, this, &MainWindow::placeNoteRows);
    return QMainWindow::eventFilter(object, event);
}

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
    if (m_noteLock)
        return;
    placeNoteRows();
    const bool on = row >= 0 && row < m_store->notes().size();
    m_editor->setEnabled(on);
    if (on)
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

void MainWindow::showStatus(const QString &text)
{
    QString line = text;
    if (!m_drift.isEmpty())
        line += QStringLiteral("    ") + m_drift;
    if (!m_conflict.isEmpty())
        line += QStringLiteral("    ") + m_conflict;
    statusBar()->showMessage(line);
}

void MainWindow::showLineStat()
{
    if (!m_stat)
        return;
    int adds = 0;
    int dels = 0;
    for (const FileDiff &file : m_doc.files) {
        adds += file.adds;
        dels += file.dels;
    }
    m_stat->setText(QStringLiteral("<span style=\"color:%1\">+%2</span>  <span style=\"color:%3\">−%4</span>")
                        .arg(kAddFg.name(), QString::number(adds), kDelFg.name(), QString::number(dels)));
}

void MainWindow::updateStatus()
{
    showLineStat();
    showStatus(QStringLiteral("%1 files    %2 comments")
                   .arg(m_doc.files.size())
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
