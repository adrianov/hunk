// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "LangDef.hpp"

// Ruby method for `spot.name`: comment above it, the def line, and the body.
// The text comes from the warm index. When a walk is stored, `done` is queued on `watch`.
void rubyWarm(const QString &root, QPointer<QObject> watch = {}, std::function<void()> done = {});
bool rubyReady(const QString &root);
LangDef rubyLookup(const LangSpot &spot);
