// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

#include <QList>

// Ruby additions on the shared lookup: which token is a name, super, columns, and how hits rank.
QString rubySeek(const QString &root, const QString &path, int line, const QString &name);
bool rubyExtra(const LangSpot &spot, LangDef *out);
LangDef rubyChoose(const QList<LangDef> &hits, const LangSpot &spot);
