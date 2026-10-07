#include "DiffFold.hpp"

namespace {

constexpr int kKeep = 3;
constexpr int kMin = 8;

void hideRange(QList<FoldSpan> *out, int from, int to, const QSet<int> &open)
{
    if (to - from < kMin)
        return;
    int cursor = from;
    while (cursor < to) {
        if (open.contains(cursor)) {
            ++cursor;
            continue;
        }
        const int first = cursor;
        while (cursor < to && !open.contains(cursor))
            ++cursor;
        if (cursor > first)
            out->push_back(FoldSpan{first, cursor - 1});
    }
}

// Keep a few unchanged lines beside each change. Hide the rest of a long run.
void foldRun(QList<FoldSpan> *out, int begin, int end, int total, const QSet<int> &open)
{
    const bool before = begin > 0;
    const bool after = end < total;
    if (!before && !after)
        return;
    const int from = begin + (before ? kKeep : 0);
    const int to = end - (after ? kKeep : 0);
    if (from < to)
        hideRange(out, from, to, open);
}

} // namespace

QList<FoldSpan> foldSpans(const QList<DiffRow> &rows, const QSet<int> &open)
{
    QList<FoldSpan> spans;
    int index = 0;
    while (index < rows.size()) {
        if (rows.at(index).kind != RowKind::Context) {
            ++index;
            continue;
        }
        const int begin = index;
        while (index < rows.size() && rows.at(index).kind == RowKind::Context)
            ++index;
        foldRun(&spans, begin, index, rows.size(), open);
    }
    return spans;
}
