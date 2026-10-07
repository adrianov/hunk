#include "SyntaxRule.hpp"

#include <cstring>

namespace {

int markBlock(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    const int from = index + int(std::strlen(rule.open));
    const int at = text.indexOf(QLatin1String(rule.close), from);
    if (at < 0) {
        addSpan(out, index, text.size(), SynKind::Comment);
        scan->block = true;
        return text.size();
    }
    addSpan(out, index, at + int(std::strlen(rule.close)), SynKind::Comment);
    return at + int(std::strlen(rule.close));
}

int takeText(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    if (const int next = eatTriple(text, index, rule, scan, out); next != index)
        return next;
    if (const int next = eatString(text, index, out); next != index)
        return next;
    if (const int next = eatBlock(text, index, rule, scan, out); next != index)
        return next;
    if (const int next = eatLine(text, index, rule, out); next != index)
        return next;
    return index;
}

int takeToken(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    if (const int next = takeText(text, index, rule, scan, out); next != index)
        return next;
    if (const int next = eatHash(text, index, rule, out); next != index)
        return next;
    if (const int next = eatTag(text, index, rule, out); next != index)
        return next;
    if (const int next = eatHex(text, index, rule, out); next != index)
        return next;
    if (const int next = eatNumber(text, index, out); next != index)
        return next;
    if (const int next = eatMark(text, index, rule, out); next != index)
        return next;
    return eatWord(text, index, rule, out);
}

void scanLine(const QString &text, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    int index = 0;
    if (scan->block)
        index = resumeBlock(text, rule, scan, out);
    else if (scan->triple)
        index = resumeString(text, scan, out);
    while (index < text.size() && !scan->block && !scan->triple) {
        const int next = takeToken(text, index, rule, scan, out);
        index = next == index ? index + 1 : next;
    }
}

} // namespace

int eatBlock(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    if (!rule.open || !text.sliced(index).startsWith(QLatin1String(rule.open)))
        return index;
    if (rule.open[0] == '=' && !text.left(index).trimmed().isEmpty())
        return index;
    return markBlock(text, index, rule, scan, out);
}

int eatLine(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if (!rule.line || !text.sliced(index).startsWith(QLatin1String(rule.line)))
        return index;
    if (index > 0 && text.at(index - 1) == QLatin1Char(':'))
        return index;
    addSpan(out, index, text.size(), SynKind::Comment);
    return text.size();
}

int eatHash(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if ((rule.flags & Preproc) == 0 || text.at(index) != QLatin1Char('#'))
        return index;
    if (!text.left(index).trimmed().isEmpty())
        return index;
    addSpan(out, index, text.size(), SynKind::Keyword);
    return text.size();
}

int eatTag(const QString &text, int index, const Rule &rule, QList<SynSpan> *out)
{
    if ((rule.flags & Html) == 0 || text.at(index) != QLatin1Char('<'))
        return index;
    const int end = text.indexOf(QLatin1Char('>'), index + 1);
    addSpan(out, index, end < 0 ? text.size() : end + 1, SynKind::Keyword);
    return end < 0 ? text.size() : end + 1;
}

int resumeBlock(const QString &text, const Rule &rule, Scan *scan, QList<SynSpan> *out)
{
    const int at = text.indexOf(QLatin1String(rule.close));
    if (at < 0) {
        addSpan(out, 0, text.size(), SynKind::Comment);
        return text.size();
    }
    addSpan(out, 0, at + int(std::strlen(rule.close)), SynKind::Comment);
    scan->block = false;
    return at + int(std::strlen(rule.close));
}

void colorSide(FileDiff *file, bool left, const Rule &rule)
{
    Scan scan;
    for (DiffRow &row : file->rows)
        scanLine(left ? row.leftText : row.rightText, rule, &scan, left ? &row.leftSyn : &row.rightSyn);
}
