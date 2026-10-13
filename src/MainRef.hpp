// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

class QAbstractItemView;
class QComboBox;
class QLineEdit;
class QPoint;

void showRefText(QComboBox *box);
void watchRefFilter(QComboBox *box);
bool refFieldMenu(QLineEdit *edit, const QPoint &global);
void armRefList(QAbstractItemView *popup);
