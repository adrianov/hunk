// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#pragma once

#include "DiffDoc.hpp"

#include <QColor>

extern QColor kBg;
extern QColor kHeaderBg;
extern QColor kGutter;
extern QColor kText;
extern QColor kMuted;
extern QColor kFile;
extern QColor kAccent;
extern QColor kAddFg;
extern QColor kDelFg;
extern QColor kAddBg;
extern QColor kDelBg;
extern QColor kAddWord;
extern QColor kDelWord;
extern QColor kEmpty;
extern QColor kLine;
extern QColor kNote;
extern QColor kSynComment;
extern QColor kSynString;
extern QColor kSynNumber;
extern QColor kSynKeyword;
extern QColor kSynType;
extern QColor kSynMethod;
extern QColor kSynVariable;

QColor synColor(SynKind kind);
void useDiffColors(bool dark);
