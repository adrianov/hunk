#include "SyntaxRule.hpp"

namespace {

bool listed(const char *const *words, QStringView word, bool fold)
{
    if (!words)
        return false;
    const QString folded = fold ? word.toString().toLower() : QString();
    for (; *words; ++words) {
        if (fold ? folded == QLatin1String(*words) : word == QLatin1String(*words))
            return true;
    }
    return false;
}

SynKind wordKind(QStringView word, const Rule &rule)
{
    if (listed(rule.words, word, (rule.flags & Fold) != 0))
        return SynKind::Keyword;
    if ((rule.flags & Caps) != 0 && word.at(0).isUpper())
        return SynKind::Type;
    return SynKind::Plain;
}

int afterSigil(const QString &text, int index, QChar ch)
{
    int cursor = index + 1;
    if (ch == QLatin1Char('@') && cursor < text.size() && text.at(cursor) == QLatin1Char('@'))
        ++cursor;
    return cursor;
}

int markEnd(const QString &text, int index, QChar ch, const Rule &rule)
{
    const int cursor = afterSigil(text, index, ch);
    if (cursor >= text.size() || !text.at(cursor).isLetter())
        return index;
    int end = cursor;
    while (end < text.size() && wordChar(text.at(end), rule))
        ++end;
    return end;
}

bool markChar(QChar ch)
{
    return ch == QLatin1Char('@') || ch == QLatin1Char('$') || ch == QLatin1Char(':');
}

bool scopeOp(const QString &text, int index, QChar ch)
{
    return ch == QLatin1Char(':') && index + 1 < text.size() && text.at(index + 1) == QLatin1Char(':');
}

} // namespace

bool wordChar(QChar ch, const Rule &rule)
{
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        return true;
    return (rule.flags & RubyId) != 0 && (ch == QLatin1Char('?') || ch == QLatin1Char('!'));
}

int eatWord(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if (!text.at(index).isLetter() && text.at(index) != QLatin1Char('_'))
        return index;
    int cursor = index + 1;
    while (cursor < text.size() && wordChar(text.at(cursor), rule))
        ++cursor;
    const SynKind kind = wordKind(QStringView(text).mid(index, cursor - index), rule);
    if (kind != SynKind::Plain)
        addSpan(out, index, cursor, kind);
    return cursor;
}

int eatMark(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if ((rule.flags & Marks) == 0)
        return index;
    const QChar ch = text.at(index);
    if (!markChar(ch) || scopeOp(text, index, ch))
        return index;
    const int end = markEnd(text, index, ch, rule);
    if (end == index)
        return index;
    addSpan(out, index, end, SynKind::Type);
    return end;
}
