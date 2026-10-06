#pragma once

#include "DiffDoc.hpp"

#include <QList>
#include <QString>

struct RawLine {
    QChar kind = QLatin1Char(' ');
    QString text;
    int left = 0;
    int right = 0;
};

struct ParseCursor {
    FileDiff file;
    QList<RawLine> lines;
    bool inHunk = false;
    bool skipping = false;
    int left = 0;
    int right = 0;
};

QString expandTabs(const QString &text);
int readPath(const QString &text, int index, QString *path);
QString pathAfter(const QString &line);
void wordDiff(const QString &left, const QString &right, QList<WordSpan> *leftSpans, QList<WordSpan> *rightSpans);
QList<DiffRow> zipRows(const QList<RawLine> &raw);
void finishFile(ParseCursor &cur, DiffDoc &doc);
bool startFile(ParseCursor &cur, DiffDoc &doc, const QString &line);
bool takeHeader(ParseCursor &cur, const QString &line);
bool takeHunk(ParseCursor &cur, const QString &line);
void takeRow(ParseCursor &cur, const QString &line);
