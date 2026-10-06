#pragma once

#include <QList>
#include <QString>

// One word-level highlight range inside a diff line.
struct WordSpan {
    int start = 0;
    int end = 0;
    bool changed = false;
};

enum class RowKind { Context, Add, Del, Mod };

// One visual row. A modification pairs an old line with a new line.
struct DiffRow {
    RowKind kind = RowKind::Context;
    int leftNum = 0;
    int rightNum = 0;
    QString leftText;
    QString rightText;
    QList<WordSpan> leftSpans;
    QList<WordSpan> rightSpans;
};

// One file from a unified diff.
struct FileDiff {
    QString oldPath;
    QString newPath;
    bool added = false;
    bool removed = false;
    bool binary = false;
    int adds = 0;
    int dels = 0;
    QList<DiffRow> rows;

    QString path() const
    {
        if (!newPath.isEmpty() && newPath != QLatin1String("/dev/null"))
            return newPath;
        return oldPath;
    }

    QString title() const
    {
        const bool oldOk = !oldPath.isEmpty() && oldPath != QLatin1String("/dev/null");
        const bool newOk = !newPath.isEmpty() && newPath != QLatin1String("/dev/null");
        if (oldOk && newOk && oldPath != newPath)
            return oldPath + QStringLiteral(" → ") + newPath;
        return path();
    }

    QString shortName() const;

    QString folder() const
    {
        const QString p = path();
        const int slash = p.lastIndexOf(QLatin1Char('/'));
        return slash >= 0 ? p.left(slash) : QString();
    }

    bool singlePane() const
    {
        if (removed || binary || rows.isEmpty())
            return false;
        if (added)
            return true;
        for (const DiffRow &row : rows) {
            if (row.kind != RowKind::Add)
                return false;
        }
        return true;
    }
};

struct DiffDoc {
    QList<FileDiff> files;
};

struct LineHit {
    int file = -1;
    int row = -1;
};
