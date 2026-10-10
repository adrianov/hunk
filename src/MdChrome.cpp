// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdView.hpp"

#include "DiffColors.hpp"
#include "MdRead.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace {

class MdBar : public QWidget {
public:
    using QWidget::QWidget;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), kHeaderBg);
        painter.fillRect(0, 0, 3, height(), kAccent);
        painter.setPen(kLine);
        painter.drawLine(0, height() - 1, width(), height() - 1);
    }
};

QPushButton *modeButton(const QString &text, const QString &tip, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tip);
    return button;
}

void placeBar(QWidget *bar, QLabel *title, QPushButton *source, QPushButton *rendered)
{
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(12, 4, 8, 4);
    lay->setSpacing(6);
    lay->addWidget(title, 1);
    lay->addWidget(source);
    lay->addWidget(rendered);
}

} // namespace

void MdView::buildBar()
{
    m_bar = new MdBar(this);
    m_bar->setAttribute(Qt::WA_OpaquePaintEvent);
    m_title = new QLabel(m_bar);
    m_title->setMinimumWidth(0);
    m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_source = modeButton(QStringLiteral("Source"), QStringLiteral("Show the diff"), m_bar);
    m_rendered = modeButton(QStringLiteral("Rendered"), QStringLiteral("Show the rendered document"), m_bar);
    m_source->setChecked(true);
    placeBar(m_bar, m_title, m_source, m_rendered);
    m_bar->hide();
}

void MdView::wireSwitch()
{
    m_modes = new QButtonGroup(this);
    m_modes->setExclusive(true);
    m_modes->addButton(m_source, 0);
    m_modes->addButton(m_rendered, 1);
    connect(m_modes, &QButtonGroup::idClicked, this, [this](int id) { choose(id); });
}

QWidget *MdView::buildPane(QTextBrowser **browser, QLabel **caption)
{
    auto *pane = new QWidget(m_page);
    auto *lay = new QVBoxLayout(pane);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    *caption = new QLabel(pane);
    (*caption)->setMinimumWidth(0);
    (*caption)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    (*caption)->setContentsMargins(12, 6, 12, 6);
    *browser = new QTextBrowser(pane);
    lay->addWidget(*caption);
    lay->addWidget(*browser, 1);
    return pane;
}

void MdView::buildPage()
{
    m_page = new QWidget(this);
    m_leftPane = buildPane(&m_left, &m_leftCaption);
    m_rightPane = buildPane(&m_right, &m_rightCaption);
    m_split = new QSplitter(m_page);
    m_split->setChildrenCollapsible(false);
    m_split->addWidget(m_leftPane);
    m_split->addWidget(m_rightPane);
    auto *lay = new QVBoxLayout(m_page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_split);
    paintGround(m_page);
}
