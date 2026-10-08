// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainSeen.hpp"

#include <QCryptographicHash>

namespace {

void addField(QCryptographicHash *hash, const QByteArray &bytes)
{
    hash->addData(bytes);
    hash->addData(QByteArray(1, '\0'));
}

QString rowSig(const DiffRow &row)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    addField(&hash, QByteArray::number(static_cast<int>(row.kind)));
    addField(&hash, row.leftText.toUtf8());
    addField(&hash, row.rightText.toUtf8());
    return QString::fromLatin1(hash.result().toHex());
}

QString fileStamp(const FileDiff &file)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    addField(&hash, file.path().toUtf8());
    addField(&hash, QByteArray::number(file.adds));
    addField(&hash, QByteArray::number(file.dels));
    addField(&hash, file.binary ? "1" : "0");
    return QString::fromLatin1(hash.result().toHex());
}

void countSigs(QHash<QString, int> *left, const QStringList &sigs)
{
    for (const QString &sig : sigs)
        (*left)[sig] = left->value(sig) + 1;
}

void claimSlot(DiffRow *row, const QString &sig, QHash<QString, int> *left)
{
    const int count = left->value(sig);
    if (count <= 0) {
        row->heat = RowHeat::Fresh;
        return;
    }
    row->heat = RowHeat::Seen;
    left->insert(sig, count - 1);
}

void claimIndexes(const QList<DiffRow *> &rows, const QStringList &sigs, const QStringList &saved, QHash<QString, int> *left)
{
    for (int index = 0; index < qMin(saved.size(), rows.size()); ++index) {
        if (saved.at(index) == sigs.at(index))
            claimSlot(rows.at(index), sigs.at(index), left);
    }
}

void claimRest(const QList<DiffRow *> &rows, const QStringList &sigs, QHash<QString, int> *left)
{
    for (int index = 0; index < rows.size(); ++index) {
        if (rows.at(index)->heat == RowHeat::Open)
            claimSlot(rows.at(index), sigs.at(index), left);
    }
}

void applyHeat(FileDiff *file, const QString &prior)
{
    QList<DiffRow *> rows;
    QStringList sigs;
    for (DiffRow &row : file->rows) {
        if (row.kind == RowKind::Context)
            continue;
        rows.push_back(&row);
        sigs.push_back(rowSig(row));
    }
    QStringList saved;
    for (const QString &sig : prior.split(QLatin1Char(','))) {
        if (!sig.isEmpty())
            saved.push_back(sig);
    }
    QHash<QString, int> left;
    countSigs(&left, saved);
    claimIndexes(rows, sigs, saved, &left);
    claimRest(rows, sigs, &left);
}

} // namespace

QString changeStamp(const FileDiff &file)
{
    QStringList sigs;
    for (const DiffRow &row : file.rows) {
        if (row.kind != RowKind::Context)
            sigs.push_back(rowSig(row));
    }
    return sigs.isEmpty() ? fileStamp(file) : sigs.join(QLatin1Char(','));
}

void applySeen(DiffDoc *doc, const QHash<QString, QString> &seen)
{
    for (FileDiff &file : doc->files) {
        const QString prior = seen.value(file.path());
        if (!prior.isEmpty())
            applyHeat(&file, prior);
    }
}
