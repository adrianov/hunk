// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffColors.hpp"

QColor kBg;
QColor kHeaderBg;
QColor kGutter;
QColor kText;
QColor kMuted;
QColor kFile;
QColor kAccent;
QColor kAddFg;
QColor kDelFg;
QColor kAddBg;
QColor kDelBg;
QColor kAddWord;
QColor kDelWord;
QColor kEmpty;
QColor kLine;
QColor kNote;
QColor kSynComment;
QColor kSynString;
QColor kSynNumber;
QColor kSynKeyword;
QColor kSynType;
QColor kSynMethod;
QColor kSynVariable;

QColor synColor(SynKind kind)
{
    switch (kind) {
    case SynKind::Comment:
        return kSynComment;
    case SynKind::String:
        return kSynString;
    case SynKind::Number:
        return kSynNumber;
    case SynKind::Keyword:
        return kSynKeyword;
    case SynKind::Type:
        return kSynType;
    case SynKind::Method:
        return kSynMethod;
    case SynKind::Variable:
        return kSynVariable;
    case SynKind::Plain:
        return kText;
    }
    return kText;
}

namespace {

struct HexSlot {
    QColor *color;
    const char *dark;
    const char *light;
};

const HexSlot kHex[] = {
    {&kBg, "#1e1e1e", "#ffffff"},       {&kHeaderBg, "#252526", "#f3f3f3"}, {&kGutter, "#252526", "#f3f3f3"},
    {&kText, "#d4d4d4", "#1e1e1e"},      {&kMuted, "#858585", "#6e6e6e"},    {&kFile, "#7eb8da", "#0451a5"},
    {&kAccent, "#0e639c", "#0078d4"},    {&kAddFg, "#3fb950", "#1a7f37"},    {&kDelFg, "#f85149", "#d1242f"},
    {&kEmpty, "#1a1a1a", "#f6f6f6"},     {&kLine, "#333333", "#e5e5e5"},     {&kNote, "#c586c0", "#a626a4"},
    {&kSynComment, "#6a9955", "#008000"}, {&kSynString, "#ce9178", "#a31515"}, {&kSynNumber, "#b5cea8", "#098658"},
    {&kSynKeyword, "#569cd6", "#0000ff"}, {&kSynType, "#4ec9b0", "#267f99"},  {&kSynMethod, "#dcdcaa", "#795e26"},
    {&kSynVariable, "#9cdcfe", "#001080"},
};

void paintHex(bool dark)
{
    for (const HexSlot &slot : kHex)
        *slot.color = QColor(QLatin1String(dark ? slot.dark : slot.light));
}

void paintMark(bool dark)
{
    if (dark) {
        kAddBg = QColor(46, 160, 67, 38);
        kDelBg = QColor(248, 81, 73, 32);
        kAddWord = QColor(46, 160, 67, 110);
        kDelWord = QColor(248, 81, 73, 110);
        return;
    }
    kAddBg = QColor(46, 160, 67, 48);
    kDelBg = QColor(207, 34, 46, 36);
    kAddWord = QColor(46, 160, 67, 90);
    kDelWord = QColor(207, 34, 46, 80);
}

} // namespace

void useDiffColors(bool dark)
{
    paintHex(dark);
    paintMark(dark);
}
