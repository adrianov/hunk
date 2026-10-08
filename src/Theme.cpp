// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainDetail.hpp"

#include "DiffColors.hpp"

#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/Theme.hpp>

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>
#include <QToolTip>

namespace {

bool g_systemDark = false;

// Tooltips use the window background and normal text, not the inverted chip colors.
class AppStyle : public oclero::qlementine::QlementineStyle {
public:
    using QlementineStyle::QlementineStyle;

    const QColor &toolTipBackgroundColor() const override
    {
        return theme().backgroundColorMain1;
    }

    const QColor &toolTipBorderColor() const override
    {
        return theme().borderColor;
    }

    const QColor &toolTipForegroundColor() const override
    {
        return theme().secondaryColor;
    }

    void triggerCompleteRepaint() override
    {
        QlementineStyle::triggerCompleteRepaint();
        QPalette palette = QToolTip::palette();
        palette.setColor(QPalette::All, QPalette::ToolTipBase, toolTipBackgroundColor());
        palette.setColor(QPalette::All, QPalette::ToolTipText, toolTipForegroundColor());
        QToolTip::setPalette(palette);
    }
};

AppStyle *appStyle()
{
    auto *style = dynamic_cast<AppStyle *>(qApp->style());
    if (style)
        return style;
    style = new AppStyle(qApp);
    qApp->setStyle(style);
    return style;
}

bool picksDark(bool forceDark)
{
    if (forceDark)
        return true;
    const ThemePick pick = themePick();
    if (pick == ThemePick::Light)
        return false;
    if (pick == ThemePick::Dark)
        return true;
    return g_systemDark;
}

} // namespace

void noteSystemTheme()
{
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark)
        g_systemDark = true;
    else if (scheme == Qt::ColorScheme::Light)
        g_systemDark = false;
    else
        g_systemDark = qApp->palette().color(QPalette::Window).lightness() < 128;
}

void applyTheme(bool forceDark)
{
    const bool dark = picksDark(forceDark);
    const auto theme = dark ? oclero::qlementine::Theme::makeDark() : oclero::qlementine::Theme::makeLight();
    appStyle()->setTheme(theme);
    useDiffColors(dark);
}

ThemePick themePick()
{
    const QString name = QSettings().value(QStringLiteral("theme"), QStringLiteral("system")).toString();
    if (name == QLatin1String("dark"))
        return ThemePick::Dark;
    if (name == QLatin1String("light"))
        return ThemePick::Light;
    return ThemePick::System;
}

void saveThemePick(ThemePick pick)
{
    const char *name = "system";
    if (pick == ThemePick::Dark)
        name = "dark";
    else if (pick == ThemePick::Light)
        name = "light";
    QSettings().setValue(QStringLiteral("theme"), QLatin1String(name));
}
