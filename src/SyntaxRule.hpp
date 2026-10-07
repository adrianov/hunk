#pragma once

#include "DiffDoc.hpp"

enum class Lang {
    None, Ruby, Py, Js, Cpp, Rust, Go, Java, Kotlin, Swift, Shell, Css, Html, Json, Yaml, Sql, Php, Lua, Elixir
};

enum RuleFlag : unsigned {
    RubyId = 1,
    Triple = 2,
    Preproc = 4,
    Caps = 8,
    Html = 16,
    Marks = 32,
    Css = 64,
    Fold = 128
};

struct Rule {
    const char *line = nullptr;
    const char *open = nullptr;
    const char *close = nullptr;
    const char *const *words = nullptr;
    unsigned flags = 0;
};

struct Scan {
    bool block = false;
    bool triple = false;
    char quote = 0;
};

Rule ruleFor(Lang lang);
Lang langFor(const QString &path);
void colorSide(FileDiff *file, bool left, const Rule &rule);

void addSpan(QList<SynSpan> *out, int start, int end, SynKind kind);
bool wordChar(QChar ch, const Rule &rule);
int eatString(const QString &text, int index, QList<SynSpan> *out);
int eatTriple(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out);
int eatNumber(const QString &text, int index, QList<SynSpan> *out);
int eatHex(const QString &text, int index, const Rule &rule, QList<SynSpan> *out);
int eatWord(const QString &text, int index, const Rule &rule, QList<SynSpan> *out);
int eatMark(const QString &text, int index, const Rule &rule, QList<SynSpan> *out);
int eatBlock(const QString &text, int index, const Rule &rule, Scan *scan, QList<SynSpan> *out);
int eatLine(const QString &text, int index, const Rule &rule, QList<SynSpan> *out);
int eatHash(const QString &text, int index, const Rule &rule, QList<SynSpan> *out);
int eatTag(const QString &text, int index, const Rule &rule, QList<SynSpan> *out);
int resumeBlock(const QString &text, const Rule &rule, Scan *scan, QList<SynSpan> *out);
int resumeString(const QString &text, Scan *scan, QList<SynSpan> *out);
