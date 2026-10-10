// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdView.hpp"

#include "DiffCanvas.hpp"
#include "DiffColors.hpp"
#include "MdHtml.hpp"
#include "MdRead.hpp"

#include <QButtonGroup>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWheelEvent>

#include <functional>

namespace {

class MdBar : public QWidget {
public:
    explicit MdBar(DiffCanvas *canvas, QWidget *parent)
        : QWidget(parent)
        , m_canvas(canvas)
    {
        setMouseTracking(true);
    }

    std::function<void()> review;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), kHeaderBg);
        painter.fillRect(0, 0, 3, height(), kAccent);
        painter.setPen(kLine);
        painter.drawLine(0, height() - 1, width(), height() - 1);
        paintReviewCheck(&painter, checkRect(), m_hot, false, kText);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && checkRect().contains(event->position().toPoint()) && review)
            review();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const bool hot = checkRect().contains(event->position().toPoint());
        if (hot == m_hot)
            return;
        m_hot = hot;
        setCursor(hot ? Qt::PointingHandCursor : Qt::ArrowCursor);
        setToolTip(hot ? QStringLiteral("Mark as reviewed") : QString());
        update();
    }

    void leaveEvent(QEvent *) override
    {
        m_hot = false;
        unsetCursor();
        setToolTip(QString());
        update();
    }

    void wheelEvent(QWheelEvent *event) override
    {
        QCoreApplication::sendEvent(m_canvas->viewport(), event);
    }

private:
    QRect checkRect() const
    {
        return QRect(10, (height() - 22) / 2, 22, 22);
    }

    DiffCanvas *m_canvas;
    bool m_hot = false;
};

QPushButton *modeButton(const QString &text, const QString &tip, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(tip);
    button->setFixedHeight(24);
    return button;
}

void placeBar(QWidget *bar, int height, QLabel *title, QLabel *counts, QPushButton *source, QPushButton *rendered)
{
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);
    counts->setTextFormat(Qt::RichText);
    bar->setFixedHeight(height);
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(40, 0, 8, 0);
    lay->setSpacing(6);
    lay->addWidget(title, 1);
    lay->addWidget(counts);
    lay->addWidget(source);
    lay->addWidget(rendered);
    source->setChecked(true);
    bar->hide();
}

} // namespace

void MdView::buildBar()
{
    auto *bar = new MdBar(m_canvas, this);
    bar->review = [this] {
        m_canvas->markReviewed(m_file);
    };
    m_bar = bar;
    m_bar->setAttribute(Qt::WA_OpaquePaintEvent);
    m_title = new QLabel(m_bar);
    m_title->setMinimumWidth(0);
    m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_counts = new QLabel(m_bar);
    m_source = modeButton(QStringLiteral("Source"), QStringLiteral("Show the diff"), m_bar);
    m_rendered = modeButton(QStringLiteral("Rendered"), QStringLiteral("Show the rendered document"), m_bar);
    placeBar(m_bar, m_canvas->headerHeight(), m_title, m_counts, m_source, m_rendered);
}

void MdView::wireSwitch()
{
    m_modes = new QButtonGroup(this);
    m_modes->setExclusive(true);
    m_modes->addButton(m_source, 0);
    m_modes->addButton(m_rendered, 1);
    connect(m_modes, &QButtonGroup::idClicked, this, [this](int id) { choose(id); });
}

QWidget *MdView::buildPane(QLabel **caption)
{
    auto *pane = new QWidget(m_page);
    auto *lay = new QVBoxLayout(pane);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    *caption = new QLabel(pane);
    (*caption)->setMinimumWidth(0);
    (*caption)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    (*caption)->setContentsMargins(12, 6, 12, 6);
    lay->addWidget(*caption);
    return pane;
}

void MdView::ensureBrowser(QWebEngineView **view, QWidget *pane)
{
    if (*view)
        return;
    *view = new QWebEngineView(pane);
    prepareMarkdown(*view);
    static_cast<QVBoxLayout *>(pane->layout())->addWidget(*view, 1);
}

void MdView::buildPage()
{
    m_page = new QWidget(this);
    m_leftPane = buildPane(&m_leftCaption);
    m_rightPane = buildPane(&m_rightCaption);
    m_split = new QSplitter(m_page);
    m_split->setChildrenCollapsible(false);
    m_split->addWidget(m_leftPane);
    m_split->addWidget(m_rightPane);
    auto *lay = new QVBoxLayout(m_page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_split);
    paintGround(m_page);
}
