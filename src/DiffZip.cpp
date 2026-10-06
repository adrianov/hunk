#include "DiffDetail.hpp"

namespace {

DiffRow fromRaw(const RawLine &raw, RowKind kind)
{
    DiffRow row;
    row.kind = kind;
    row.leftNum = raw.left;
    row.rightNum = raw.right;
    row.leftText = raw.text;
    row.rightText = raw.text;
    if (kind == RowKind::Add)
        row.leftText.clear();
    if (kind == RowKind::Del)
        row.rightText.clear();
    return row;
}

DiffRow modifiedRow(const RawLine &del, const RawLine &add)
{
    DiffRow row;
    row.kind = RowKind::Mod;
    row.leftNum = del.left;
    row.rightNum = add.right;
    row.leftText = del.text;
    row.rightText = add.text;
    wordDiff(row.leftText, row.rightText, &row.leftSpans, &row.rightSpans);
    return row;
}

void appendChange(QList<DiffRow> *rows, const QList<RawLine> &dels, const QList<RawLine> &adds, int pair)
{
    const bool hasDel = pair < dels.size();
    const bool hasAdd = pair < adds.size();
    if (hasDel && hasAdd)
        rows->push_back(modifiedRow(dels.at(pair), adds.at(pair)));
    else if (hasDel)
        rows->push_back(fromRaw(dels.at(pair), RowKind::Del));
    else
        rows->push_back(fromRaw(adds.at(pair), RowKind::Add));
}

void collectRun(const QList<RawLine> &raw, int *index, QChar kind, QList<RawLine> *out)
{
    while (*index < raw.size() && raw.at(*index).kind == kind)
        out->push_back(raw.at((*index)++));
}

void zipChange(QList<DiffRow> *rows, const QList<RawLine> &raw, int *index)
{
    QList<RawLine> dels;
    QList<RawLine> adds;
    collectRun(raw, index, QLatin1Char('-'), &dels);
    collectRun(raw, index, QLatin1Char('+'), &adds);
    for (int pair = 0; pair < qMax(dels.size(), adds.size()); ++pair)
        appendChange(rows, dels, adds, pair);
}

} // namespace

QList<DiffRow> zipRows(const QList<RawLine> &raw)
{
    QList<DiffRow> rows;
    for (int index = 0; index < raw.size();) {
        if (raw.at(index).kind == QLatin1Char(' ')) {
            rows.push_back(fromRaw(raw.at(index), RowKind::Context));
            ++index;
            continue;
        }
        zipChange(&rows, raw, &index);
    }
    return rows;
}
