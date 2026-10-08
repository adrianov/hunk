#pragma once

#include "ReviewNote.hpp"

class QLabel;
class QWidget;

enum class ThemePick { System, Dark, Light };

void noteSystemTheme();
void applyTheme(bool forceDark = false);
ThemePick themePick();
void saveThemePick(ThemePick pick);
QString noteLabel(const ReviewNote &note);
QLabel *rowLabel(const QWidget *row);
