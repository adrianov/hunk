// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

// Ruby method, class, or declaration for `spot.name`.
// The search runs when that name is hovered. When the root changes, `done` is queued on `watch`.
void rubyWarm(const QString &root, QPointer<QObject> watch = {}, std::function<void()> done = {});
bool rubyReady(const QString &root);
LangDef rubyLookup(const LangSpot &spot);
