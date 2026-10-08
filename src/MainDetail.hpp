// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "ReviewNote.hpp"

#include <QIcon>

class QComboBox;
class QPushButton;
class QLabel;
class QWidget;

enum class ThemePick { System, Dark, Light };
enum class ButtonIcon { Open, Refresh, Copy };

QIcon buttonIcon(ButtonIcon kind);
void setButtonIcon(QPushButton *button, const QIcon &icon);

void noteSystemTheme();
void applyTheme(bool forceDark = false);
ThemePick themePick();
void saveThemePick(ThemePick pick);
QString noteLabel(const ReviewNote &note);
QLabel *rowLabel(const QWidget *row);
void useTextColor(QWidget *widget);
void showRefText(QComboBox *box);
