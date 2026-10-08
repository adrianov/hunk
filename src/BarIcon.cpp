// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainDetail.hpp"

#include <oclero/qlementine/utils/IconUtils.hpp>

#include <QPushButton>
#include <QStyle>

namespace {

QByteArray strokeSvg(const char *body)
{
    return QByteArray(R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="#000" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">)")
        + body + "</svg>";
}

const char *iconBody(ButtonIcon kind)
{
    if (kind == ButtonIcon::Refresh)
        return R"(<path d="M21 12a9 9 0 1 1-2.64-6.36L21 8"/><path d="M21 3v5h-5"/>)";
    if (kind == ButtonIcon::Copy)
        return R"(<rect x="8" y="8" width="12" height="12" rx="2"/><path d="M4 16V6a2 2 0 0 1 2-2h10"/>)";
    return R"(<path d="M3 8a1 1 0 0 1 1-1h5l2 2h9a1 1 0 0 1 1 1v8a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1z"/>)";
}

} // namespace

QIcon buttonIcon(ButtonIcon kind)
{
    return oclero::qlementine::makeIconFromSvgData(strokeSvg(iconBody(kind)), QSize(16, 16));
}

void setButtonIcon(QPushButton *button, const QIcon &icon)
{
    button->setIcon(icon);
    const int side = button->style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, button);
    button->setIconSize(QSize(side, side));
}
