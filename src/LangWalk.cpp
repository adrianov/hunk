// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangTune.hpp"
#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

QString readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

QStringList textLines(const QString &text)
{
    QStringList lines = text.split(QLatin1Char('\n'));
    for (QString &line : lines) {
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
    }
    return lines;
}

bool skipDir(const QString &name)
{
    static const char *names[] = {"vendor", "tmp",        "node_modules", ".git",  "log",
                                  "coverage", "storage", "public",        "build", "dist", nullptr};
    for (const char *const *item = names; *item; ++item) {
        if (name == QLatin1String(*item))
            return true;
    }
    return false;
}

bool wordChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool wordEdge(const QString &text, int at, int end)
{
    const bool left = at == 0 || !wordChar(text.at(at - 1));
    const bool right = end >= text.size() || !wordChar(text.at(end));
    return left && right;
}

bool hasWord(const QString &text, const QString &name)
{
    if (name.isEmpty())
        return false;
    int from = 0;
    while (from < text.size()) {
        const int at = text.indexOf(name, from);
        if (at < 0)
            return false;
        const int end = at + name.size();
        if (wordEdge(text, at, end))
            return true;
        from = end;
    }
    return false;
}

bool wordIn(const QString &text, const QString &name)
{
    if (hasWord(text, name))
        return true;
    const int cut = name.lastIndexOf(QStringLiteral("::"));
    return cut >= 0 && hasWord(text, name.mid(cut + 2));
}

QStringList namesIn(const QString &text, const QStringList &names)
{
    QStringList hit;
    for (const QString &name : names) {
        if (wordIn(text, name))
            hit.append(name);
    }
    return hit;
}

void appendNamed(const QStringList &names, const QHash<QString, QList<LangDef>> &local,
                 QHash<QString, QList<LangDef>> *out)
{
    for (const QString &name : names) {
        const QList<LangDef> found = local.value(name);
        if (!found.isEmpty())
            (*out)[name].append(found);
    }
}

void noteFile(const QString &root, const QString &full, const QStringList &names, const LangTune *tune,
              QHash<QString, QList<LangDef>> *out, QHash<QString, QList<LangDef>> *uses)
{
    const QString text = readFile(full);
    const QStringList hit = namesIn(text, names);
    if (hit.isEmpty())
        return;
    const QString path = QDir(root).relativeFilePath(full);
    const QStringList lines = textLines(text);
    rubyKeepLines(root, path, lines);
    QHash<QString, QList<LangDef>> local;
    tune->note(lines, path, &local);
    appendNamed(hit, local, out);
    if (!uses || !tune->use)
        return;
    local.clear();
    tune->use(lines, path, hit, &local);
    appendNamed(hit, local, uses);
}

void walkFiles(const QString &root, const QString &dir, const QStringList &names, QHash<QString, QList<LangDef>> *out,
               QHash<QString, QList<LangDef>> *uses, int depth)
{
    if (depth > 24)
        return;
    const QDir folder(dir);
    for (const QFileInfo &info : folder.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (info.isDir()) {
            if (!skipDir(info.fileName()))
                walkFiles(root, info.filePath(), names, out, uses, depth + 1);
            continue;
        }
        const LangTune *tune = langTune(langFor(info.fileName()));
        if (tune && tune->note)
            noteFile(root, info.filePath(), names, tune, out, uses);
    }
}

} // namespace

void langWalk(const QString &root, const QStringList &names, QHash<QString, QList<LangDef>> *out,
             QHash<QString, QList<LangDef>> *uses)
{
    walkFiles(root, root, names, out, uses, 0);
}
