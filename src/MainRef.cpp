#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QFontMetrics>
#include <QLineEdit>
#include <QScreen>
#include <QStyleOptionComboBox>

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

void fitRefBox(QComboBox *box)
{
    const int width = refWidth(box);
    box->setMinimumWidth(width);
    box->setMaximumWidth(width);
    const QScreen *screen = box->screen();
    const int limit = screen ? screen->availableGeometry().width() : width + 1;
    box->view()->setTextElideMode(width < limit ? Qt::ElideNone : Qt::ElideRight);
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
}
