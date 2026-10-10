// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainRef.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QStyleOptionComboBox>
#include <QTimer>

void showMatches(QComboBox *box, const QString &prefix)
{
    QCompleter *found = box->completer();
    QAbstractItemView *popup = found->popup();
    popup->setMinimumWidth(box->width());
    popup->setPalette(box->palette());
    popup->viewport()->setPalette(box->palette());
    found->setCompletionPrefix(prefix);
    if (!found->setCurrentRow(0)) {
        popup->hide();
        return;
    }
    found->complete();
}

class RefPress : public QObject {
public:
    explicit RefPress(QComboBox *box)
        : QObject(box)
        , m_box(box)
    {
        connect(box->lineEdit(), &QLineEdit::editingFinished, this, [this] {
            if (m_open)
                m_saved = m_box->currentText();
        });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (trackEdit(event))
            return true;
        if (watched != m_box)
            return editPress(watched, event);
        if (event->type() == QEvent::FocusOut && listShown())
            return true;
        return arrowPress(event);
    }

private:
    bool onArrow(const QPoint &pos) const
    {
        QStyleOptionComboBox option;
        option.initFrom(m_box);
        option.editable = true;
        option.subControls = QStyle::SC_All;
        return m_box->style()
            ->subControlRect(QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, m_box)
            .contains(pos);
    }

    bool arrowPress(QEvent *event)
    {
        if (event->type() != QEvent::MouseButtonPress)
            return false;
        auto *mouse = static_cast<const QMouseEvent *>(event);
        if (mouse->button() != Qt::LeftButton || !onArrow(mouse->position().toPoint()))
            return false;
        m_box->lineEdit()->setFocus();
        m_box->lineEdit()->selectAll();
        showMatches(m_box, QString());
        return true;
    }

    bool trackEdit(QEvent *event)
    {
        const auto type = event->type();
        if (type == QEvent::FocusIn)
            remember();
        else if (type == QEvent::FocusOut && !listShown()
                 && static_cast<const QFocusEvent *>(event)->reason() != Qt::PopupFocusReason)
            m_open = false;
        else if (type == QEvent::KeyPress && static_cast<const QKeyEvent *>(event)->key() == Qt::Key_Escape)
            return cancelEdit();
        return false;
    }

    bool listShown() const
    {
        const QCompleter *found = m_box->completer();
        return found && found->popup()->isVisible();
    }

    void remember()
    {
        if (m_open)
            return;
        m_saved = m_box->currentText();
        m_open = true;
    }

    bool cancelEdit()
    {
        if (!m_open)
            return false;
        QLineEdit *edit = m_box->lineEdit();
        if (QCompleter *found = m_box->completer())
            found->popup()->hide();
        edit->setText(m_saved);
        edit->deselect();
        edit->setToolTip(m_saved);
        m_open = false;
        edit->clearFocus();
        return true;
    }

    bool editPress(QObject *watched, QEvent *event)
    {
        if (event->type() != QEvent::MouseButtonPress || m_box->hasFocus())
            return false;
        if (static_cast<const QMouseEvent *>(event)->button() != Qt::LeftButton)
            return false;
        QTimer::singleShot(0, static_cast<QLineEdit *>(watched), &QLineEdit::selectAll);
        return false;
    }

    QComboBox *m_box;
    QString m_saved;
    bool m_open = false;
};

void tuneRefPopup(QCompleter *found)
{
    found->setModelSorting(QCompleter::UnsortedModel);
    found->setCaseSensitivity(Qt::CaseInsensitive);
    found->setFilterMode(Qt::MatchContains);
    found->setCompletionMode(QCompleter::PopupCompletion);
    found->setMaxVisibleItems(24);
    QAbstractItemView *popup = found->popup();
    popup->setFocusPolicy(Qt::NoFocus);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);
    popup->setWindowFlag(Qt::WindowDoesNotAcceptFocus);
    popup->setTextElideMode(Qt::ElideNone);
    popup->setAutoFillBackground(true);
    popup->viewport()->setAutoFillBackground(true);
}

void watchRefFilter(QComboBox *box)
{
    tuneRefPopup(box->completer());
    QLineEdit *edit = box->lineEdit();
    edit->show();
    edit->setCursor(Qt::IBeamCursor);
    auto *press = new RefPress(box);
    edit->installEventFilter(press);
    box->installEventFilter(press);
    QObject::connect(edit, &QLineEdit::textEdited, box, [box](const QString &text) { showMatches(box, text); });
    showRefText(box);
}
