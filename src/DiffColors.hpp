#pragma once

#include "DiffDoc.hpp"

#include <QColor>

extern const QColor kBg;
extern const QColor kHeaderBg;
extern const QColor kGutter;
extern const QColor kText;
extern const QColor kMuted;
extern const QColor kFile;
extern const QColor kAccent;
extern const QColor kAddFg;
extern const QColor kDelFg;
extern const QColor kAddBg;
extern const QColor kDelBg;
extern const QColor kAddWord;
extern const QColor kDelWord;
extern const QColor kEmpty;
extern const QColor kLine;
extern const QColor kNote;
extern const QColor kSynComment;
extern const QColor kSynString;
extern const QColor kSynNumber;
extern const QColor kSynKeyword;
extern const QColor kSynType;

QColor synColor(SynKind kind);
