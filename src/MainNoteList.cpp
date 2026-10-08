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
    layout->setContentsMargins(2, 0, 2, 0);
    layout->setSpacing(6);
    auto *text = noteText(note, row);
    auto *drop = noteDelete(row);
    layout->addWidget(text, 1);
    layout->addWidget(drop);
    row->setProperty("noteText", QVariant::fromValue(static_cast<QObject *>(text)));
    row->setToolTip(note.body);
    row->installEventFilter(new PickRow(list, item, row));
    return {row, drop};
}

struct NoteId {
    QString path;
    bool oldSide = false;
    int line = -1;
};

NoteId noteId(const ReviewNote &note)
{
    return {note.path, note.oldSide, note.line};
}

void tagId(QListWidgetItem *item, const NoteId &id)
{
    item->setData(Qt::UserRole, id.path);
    item->setData(Qt::UserRole + 1, id.oldSide);
    item->setData(Qt::UserRole + 2, id.line);
}

NoteId selectedId(const QListWidget *list)
{
    const QListWidgetItem *current = list->currentItem();
    if (!current)
        return {};
    return {current->data(Qt::UserRole).toString(), current->data(Qt::UserRole + 1).toBool(),
            current->data(Qt::UserRole + 2).toInt()};
}

int rowFor(const QListWidget *list, const NoteId &id)
{
    if (id.line <= 0)
        return -1;
    for (int index = 0; index < list->count(); ++index) {
        const QListWidgetItem *item = list->item(index);
        if (item->data(Qt::UserRole).toString() == id.path && item->data(Qt::UserRole + 1).toBool() == id.oldSide
            && item->data(Qt::UserRole + 2).toInt() == id.line)
            return index;
    }
    return -1;
}

void dropId(ReviewStore *store, const NoteId &id)
{
    const QList<ReviewNote> &notes = store->notes();
    for (int index = 0; index < notes.size(); ++index) {
        const ReviewNote &note = notes.at(index);
        if (note.path == id.path && note.oldSide == id.oldSide && note.line == id.line) {
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

void MainWindow::restoreNoteRow(int row)
{
    if (row >= 0 && row < m_notes->count())
        m_notes->setCurrentRow(row);
    else {
        m_editor->clear();
        m_editor->setEnabled(false);
    }
}

void MainWindow::refreshNotes()
{
    m_noteLock = true;
    const NoteId picked = selectedId(m_notes);
    m_notes->clear();
    for (int index = 0; index < m_store->notes().size(); ++index)
        addListedNote(index);
    restoreNoteRow(rowFor(m_notes, picked));
    placeNoteRows();
    m_noteLock = false;
    const int comments = m_store->notes().size();
    m_dock->setWindowTitle(comments == 0 ? QStringLiteral("Reviews") : QStringLiteral("Reviews (%1)").arg(comments));
}
