// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTimer>

void showRefText(QComboBox *box);

class RefPress : public QObject {
public:
    explicit RefPress(QComboBox *box)
        : QObject(box->lineEdit())
        , m_box(box)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress)
            m_box->completer()->popup()->setMinimumWidth(m_box->width());
        if (event->type() != QEvent::MouseButtonPress || m_box->lineEdit()->hasFocus())
            return false;
        if (static_cast<const QMouseEvent *>(event)->button() != Qt::LeftButton)
            return false;
        QTimer::singleShot(0, static_cast<QLineEdit *>(watched), &QLineEdit::selectAll);
        return false;
    }

private:
    QComboBox *m_box;
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
    popup->setTextElideMode(Qt::ElideNone);
}

void watchRefFilter(QComboBox *box)
{
    tuneRefPopup(box->completer());
    QLineEdit *edit = box->lineEdit();
    edit->installEventFilter(new RefPress(box));
    QObject::connect(edit, &QLineEdit::textEdited, box, [box](const QString &text) {
        QCompleter *found = box->completer();
        found->popup()->setMinimumWidth(box->width());
        found->setCompletionPrefix(text);
        found->complete();
    });
    showRefText(box);
}
