#include "DiffColors.hpp"

const QColor kBg(QStringLiteral("#1e1e1e"));
const QColor kHeaderBg(QStringLiteral("#252526"));
const QColor kGutter(QStringLiteral("#252526"));
const QColor kText(QStringLiteral("#d4d4d4"));
const QColor kMuted(QStringLiteral("#858585"));
const QColor kFile(QStringLiteral("#7eb8da"));
const QColor kAccent(QStringLiteral("#0e639c"));
const QColor kAddFg(QStringLiteral("#3fb950"));
const QColor kDelFg(QStringLiteral("#f85149"));
const QColor kAddBg(46, 160, 67, 38);
const QColor kDelBg(248, 81, 73, 32);
const QColor kAddWord(46, 160, 67, 110);
const QColor kDelWord(248, 81, 73, 110);
const QColor kEmpty(QStringLiteral("#1a1a1a"));
const QColor kLine(QStringLiteral("#333333"));
const QColor kNote(QStringLiteral("#c586c0"));
const QColor kSynComment(QStringLiteral("#6a9955"));
const QColor kSynString(QStringLiteral("#ce9178"));
const QColor kSynNumber(QStringLiteral("#b5cea8"));
const QColor kSynKeyword(QStringLiteral("#569cd6"));
const QColor kSynType(QStringLiteral("#4ec9b0"));
const QColor kSynMethod(QStringLiteral("#dcdcaa"));
const QColor kSynVariable(QStringLiteral("#9cdcfe"));

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
