// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffPopup.hpp"

#include "DefCode.hpp"
#include "DiffColors.hpp"
#include "DiffDetail.hpp"

#include <QGuiApplication>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

namespace {

QString defText(const LangDef &def)
{
    QString text;
    if (!def.comment.isEmpty())
        text = def.comment + QLatin1Char('\n');
    text += def.signature;
    if (!def.body.isEmpty())
        text += QLatin1Char('\n') + def.body;
    return dedent(text);
}

void tipStyle(QFrame *frame)
{
    frame->setStyleSheet(QStringLiteral("DefPopup { background: %1; border: 1px solid %2; }").arg(kBg.name(), kLine.name()));
}

int fitAxis(int cursor, int size, int low, int high, int gap)
{
    const int at = cursor + gap;
    if (at + size <= high)
        return at;
    return qMax(low, cursor - size - gap);
}

QPoint fitPoint(const QPoint &global, const QSize &size, const QRect &area)
{
    if (area.isNull())
        return global + QPoint(14, 18);
    return QPoint(fitAxis(global.x(), size.width(), area.left(), area.right(), 14),
                  fitAxis(global.y(), size.height(), area.top(), area.bottom(), 18));
}

QScrollArea *codeScroll(DefCode *code, QWidget *parent)
{
    auto *scroll = new QScrollArea(parent);
    scroll->setWidget(code);
    scroll->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(false);
    scroll->setFocusPolicy(Qt::WheelFocus);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));
    scroll->viewport()->setAutoFillBackground(false);
    return scroll;
}

} // namespace

DefPopup::DefPopup(QWidget *parent)
    : QFrame(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);
    m_path = new QLabel(this);
    m_path->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_code = new DefCode;
    m_scroll = codeScroll(m_code, this);
    layout->addWidget(m_path);
    layout->addWidget(m_scroll);
}

void DefPopup::present(const LangDef &def, const QFont &font, const QPoint &global, const QString &from)
{
    tipStyle(this);
    const bool here = def.path == from;
    const QString line = QString::number(def.line);
    m_path->setText(here ? QStringLiteral("Same file:") + line : def.path + QLatin1Char(':') + line);
    m_path->setStyleSheet(QStringLiteral("color: %1; background: transparent;").arg((here ? kText : kMuted).name()));
    fitCode(font, defText(def), def.path);
    place(global);
    show();
}

void DefPopup::fitCode(const QFont &font, const QString &code, const QString &path)
{
    m_scroll->setFixedSize(m_code->showText(code, path, font, m_scroll->verticalScrollBar()->sizeHint().width()));
    adjustSize();
}

void DefPopup::place(const QPoint &global)
{
    const QScreen *screen = QGuiApplication::screenAt(global);
    move(fitPoint(global, size(), screen ? screen->availableGeometry() : QRect()));
}

void DefPopup::leaveEvent(QEvent *event)
{
    QFrame::leaveEvent(event);
    if (onLeave)
        onLeave();
}
