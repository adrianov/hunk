// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainDetail.hpp"

#include "DiffColors.hpp"

#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/Theme.hpp>

#include <QApplication>
#include <QEvent>
#include <QPainter>
#include <QPalette>
#include <QSettings>
#include <QStyleHints>
#include <QStyleOption>
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

    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                  QStyleHintReturn *data) const override
    {
        if (hint == SH_ComboBox_Popup)
            return 0;
        return QlementineStyle::styleHint(hint, option, widget, data);
    }

    // The menu panel is drawn only when SH_ComboBox_Popup is set, and that hint hides the scrollbar.
    void polish(QWidget *widget) override
    {
        QlementineStyle::polish(widget);
        if (!widget->inherits("QComboBoxPrivateContainer") || widget->property("hunkMenu").toBool())
            return;
        widget->setProperty("hunkMenu", true);
        widget->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() != QEvent::Paint || !watched->property("hunkMenu").toBool())
            return QlementineStyle::eventFilter(watched, event);
        auto *popup = static_cast<QWidget *>(watched);
        QPainter painter(popup);
        QStyleOption option;
        option.initFrom(popup);
        drawPrimitive(PE_PanelMenu, &option, &painter, popup);
        return true;
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
