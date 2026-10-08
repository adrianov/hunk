#pragma once

#include "ReviewNote.hpp"

enum class ThemePick { System, Dark, Light };

void noteSystemTheme();
void applyTheme(bool forceDark = false);
ThemePick themePick();
void saveThemePick(ThemePick pick);
QString noteLabel(const ReviewNote &note);
