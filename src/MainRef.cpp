// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QScreen>
#include <QStyleOptionComboBox>
#include <QToolBar>

namespace {

int widestText(const QComboBox *box)
{
    const QFontMetrics metrics(box->font());
    int text = metrics.horizontalAdvance(box->currentText());
    for (int index = 0; index < box->count(); ++index)
        text = qMax(text, metrics.horizontalAdvance(box->itemText(index)));
    return text;
}

int comboWidth(const QComboBox *box, int text)
{
    QStyleOptionComboBox option;
    option.initFrom(box);
    option.editable = box->isEditable();
    const QSize textSize(text, box->fontMetrics().height());
    return box->style()->sizeFromContents(QStyle::CT_ComboBox, &option, textSize, box).width();
}

int refWidth(const QComboBox *box)
{
    const int want = comboWidth(box, widestText(box));
    const QScreen *screen = box->screen();
    const int limit = screen ? screen->availableGeometry().width() : want;
    return qMin(qMax(want, 160), qMax(limit, 160));
}

} // namespace

int refCap(const QWidget *bar, int want)
{
    if (!bar || bar->width() <= 320)
        return want;
    return qMax(160, bar->width() / 3);
}

void showRefText(QComboBox *box)
{
    if (!box)
        return;
    useTextColor(box->lineEdit());
}

void finishRefEdit(QComboBox *box)
{
    showRefText(box);
    QLineEdit *edit = box->lineEdit();
    if (!edit)
        return;
    edit->deselect();
    edit->setToolTip(edit->text());
}

void fitRefBox(QComboBox *box)
{
    const int want = refWidth(box);
    const int width = qBound(160, want, refCap(box->parentWidget(), want));
    box->setMinimumWidth(width);
    box->setMaximumWidth(width);
    box->view()->setTextElideMode(Qt::ElideNone);
    finishRefEdit(box);
}

void watchRefFilter(QComboBox *box)
{
    QCompleter *found = box->completer();
    found->setCompletionMode(QCompleter::PopupCompletion);
    found->setFilterMode(Qt::MatchContains);
    found->setCaseSensitivity(Qt::CaseInsensitive);
    found->setModelSorting(QCompleter::UnsortedModel);
    found->popup()->setFocusPolicy(Qt::NoFocus);
    found->popup()->setTextElideMode(Qt::ElideNone);
    QObject::connect(box->lineEdit(), &QLineEdit::textEdited, box, [box] {
        box->completer()->popup()->setMinimumWidth(box->width());
    });
    showRefText(box);
}

void addRef(QToolBar *bar, QComboBox **box, const QString &label, const QString &tip)
{
    auto *caption = new QLabel(label, bar);
    useTextColor(caption);
    bar->addWidget(caption);
    *box = new QComboBox(bar);
    (*box)->setEditable(true);
    (*box)->setMinimumWidth(160);
    (*box)->setToolTip(tip);
    bar->addWidget(*box);
}

class ModePress : public QObject {
public:
    explicit ModePress(QComboBox *box)
        : QObject(box)
        , m_box(box)
    {
    }

protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::MouseButtonPress)
            return open(static_cast<QMouseEvent *>(event));
        if (event->type() == QEvent::KeyPress)
            return step(static_cast<QKeyEvent *>(event));
        return false;
    }

private:
    bool open(const QMouseEvent *event)
    {
        if (event->button() != Qt::LeftButton)
            return false;
        m_box->showPopup();
        return true;
    }

    bool step(const QKeyEvent *event)
    {
        const int key = event->key();
        if (key != Qt::Key_Down && key != Qt::Key_Up)
            return false;
        const int next = m_box->currentIndex() + (key == Qt::Key_Down ? 1 : -1);
        if (next >= 0 && next < m_box->count())
            m_box->setCurrentIndex(next);
        return true;
    }

    QComboBox *m_box;
};

void armMode(QComboBox *box)
{
    box->setEditable(true);
    box->setInsertPolicy(QComboBox::NoInsert);
    box->setCompleter(nullptr);
    QLineEdit *edit = box->lineEdit();
    edit->setReadOnly(true);
    edit->setCursor(Qt::ArrowCursor);
    useTextColor(edit);
    QObject::connect(edit, &QLineEdit::selectionChanged, edit, &QLineEdit::deselect);
    edit->installEventFilter(new ModePress(box));
}

void MainWindow::addModeBox(QToolBar *bar)
{
    m_mode = new QComboBox(this);
    const QStringList labels{QStringLiteral("Merge request"), QStringLiteral("Uncommitted"), QStringLiteral("Staged")};
    const QStringList tips{QStringLiteral("Three-dot diff from the base, including uncommitted changes"),
                           QStringLiteral("Staged and unstaged changes against HEAD"),
                           QStringLiteral("Staged changes only")};
    for (int index = 0; index < labels.size(); ++index) {
        m_mode->addItem(labels.at(index));
        m_mode->setItemData(index, tips.at(index), Qt::ToolTipRole);
    }
    armMode(m_mode);
    bar->addWidget(m_mode);
}

void MainWindow::addRefBoxes(QToolBar *bar)
{
    addRef(bar, &m_base, QStringLiteral("Base"),
           QStringLiteral("Base of the merge request. Branching point is where this branch left the default branch. Type to filter."));
    addRef(bar, &m_head, QStringLiteral("Branch"),
           QStringLiteral("Branch compared with the base. Type to filter the list."));
    watchRefFilter(m_base);
    watchRefFilter(m_head);
}
