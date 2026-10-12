// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

bool receiverChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('@');
}

int constHead(const QString &text, int at)
{
    while (at >= 2 && text.at(at - 1) == QLatin1Char(':') && text.at(at - 2) == QLatin1Char(':')) {
        at -= 2;
        while (at > 0 && receiverChar(text.at(at - 1)))
            --at;
    }
    return at;
}

int classTail(const QString &text, int at)
{
    if (at < 2 || text.at(at - 1) != QLatin1Char('.'))
        return at;
    int left = at - 1;
    while (left > 0 && receiverChar(text.at(left - 1)))
        --left;
    left = constHead(text, left);
    return left < at - 1 && text.at(left).isUpper() ? left : at;
}

} // namespace

QString rubyReceiver(const QString &text, int start)
{
    int tail = start - 1;
    if (tail > 0 && text.at(tail - 1) == QLatin1Char('&'))
        --tail;
    int at = tail;
    while (at > 0 && receiverChar(text.at(at - 1)))
        --at;
    at = classTail(text, constHead(text, at));
    return at == tail ? QString() : text.mid(at, tail - at);
}
