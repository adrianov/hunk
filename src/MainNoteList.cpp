// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffColors.hpp"
#include "MainDetail.hpp"
#include "ReviewStore.hpp"

#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVariant>

namespace {

class PickRow : public QObject {
public:
    PickRow(QListWidget *list, QListWidgetItem *item, QObject *parent)
        : QObject(parent), m_list(list), m_item(item) {}

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::MouseButtonPress && !qobject_cast<QPushButton *>(object))
            m_list->setCurrentItem(m_item);
        return false;
    }

private:
    QListWidget *m_list;
    QListWidgetItem *m_item;
};

QLabel *noteText(const ReviewNote &note, QWidget *parent)
{
    auto *label = new QLabel(noteLabel(note), parent);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setMinimumWidth(0);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    if (!note.inDiff) {
        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, kMuted);
        label->setPalette(palette);
    }
    return label;
}

QPushButton *noteDelete(QWidget *parent)
{
    auto *button = new QPushButton(QStringLiteral("×"), parent);
    button->setObjectName(QStringLiteral("noteDelete"));
    button->setFixedSize(22, 22);
    button->setToolTip(QStringLiteral("Delete"));
    return button;
}

struct ListedRow {
    QWidget *row = nullptr;
    QPushButton *drop = nullptr;
};

ListedRow noteRow(QListWidget *list, QListWidgetItem *item, const ReviewNote &note)
{
    auto *row = new QWidget;
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 4, 8, 4);
    layout->setSpacing(6);
    auto *text = noteText(note, row);
    auto *drop = noteDelete(row);
    layout->addWidget(drop);
    layout->addWidget(text, 1);
    row->setProperty("noteText", QVariant::fromValue(static_cast<QObject *>(text)));
    row->setToolTip(note.body);
    row->installEventFilter(new PickRow(list, item, row));
    return {row, drop};
}

struct NoteId {
    QString path;
    bool oldSide = false;
    int line = -1;
    int end = 0;
};

NoteId noteId(const ReviewNote &note)
{
    return {note.path, note.oldSide, note.line, note.end > note.line ? note.end : 0};
}

void tagId(QListWidgetItem *item, const NoteId &id)
{
    item->setData(Qt::UserRole, id.path);
    item->setData(Qt::UserRole + 1, id.oldSide);
    item->setData(Qt::UserRole + 2, id.line);
    item->setData(Qt::UserRole + 3, id.end);
}

NoteId selectedId(const QListWidget *list)
{
    const QListWidgetItem *current = list->currentItem();
    if (!current)
        return {};
    return {current->data(Qt::UserRole).toString(), current->data(Qt::UserRole + 1).toBool(),
            current->data(Qt::UserRole + 2).toInt(), current->data(Qt::UserRole + 3).toInt()};
}

void dropId(ReviewStore *store, const NoteId &id)
{
    const QList<ReviewNote> &notes = store->notes();
    for (int index = 0; index < notes.size(); ++index) {
        const ReviewNote &note = notes.at(index);
        const int end = note.end > note.line ? note.end : 0;
        if (note.path == id.path && note.oldSide == id.oldSide && note.line == id.line && end == id.end) {
            store->removeAt(index);
            return;
        }
    }
}

} // namespace

QLabel *rowLabel(const QWidget *row)
{
    return qobject_cast<QLabel *>(row->property("noteText").value<QObject *>());
}

void MainWindow::addListedNote(int index)
{
    const ReviewNote note = m_store->notes().at(index);
    auto *item = new QListWidgetItem();
    const NoteId id = noteId(note);
    tagId(item, id);
    const ListedRow listed = noteRow(m_notes, item, note);
    item->setSizeHint(QSize(0, listed.row->sizeHint().height()));
    m_notes->addItem(item);
    m_notes->setItemWidget(item, listed.row);
    connect(listed.drop, &QPushButton::clicked, this, [this, id]() { dropId(m_store, id); }, Qt::QueuedConnection);
}

void MainWindow::restoreNoteRow(int row, bool editing)
{
    if (row < 0 || row >= m_notes->count()) {
        m_editor->clear();
        m_editor->setEnabled(false);
        return;
    }
    m_notes->setCurrentRow(row);
    if (editing)
        m_editor->setFocus();
}

void MainWindow::jumpNote(QListWidgetItem *item)
{
    if (item)
        showNote(m_notes->row(item));
}

void MainWindow::fillNotes()
{
    connect(m_notes, &QListWidget::itemDoubleClicked, this, &MainWindow::jumpNote, Qt::UniqueConnection);
    m_notes->clear();
    for (int index = 0; index < m_store->notes().size(); ++index)
        addListedNote(index);
    const int comments = m_store->notes().size();
    m_dock->setWindowTitle(comments == 0 ? QStringLiteral("Reviews") : QStringLiteral("Reviews (%1)").arg(comments));
}

void MainWindow::refreshNotes()
{
    const bool editing = m_editor->hasFocus();
    const QString typed = m_editor->toPlainText();
    const int slot = m_notes->currentRow();
    const int oldCount = m_notes->count();
    const NoteId picked = selectedId(m_notes);
    m_noteLock = true;
    fillNotes();
    restoreNoteRow(keptNote(slot, oldCount, picked.path, picked.oldSide, picked.line, picked.end, typed, editing),
                   editing);
    placeNoteRows();
    m_noteLock = false;
}
