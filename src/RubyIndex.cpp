// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include "SyntaxRule.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QMutex>
#include <QSet>

namespace {

QString rubyRead(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
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

bool wordIn(const QString &text, const QString &name)
{
    int from = 0;
    while (from < text.size()) {
        const int at = text.indexOf(name, from);
        if (at < 0)
            return false;
        const bool left = at == 0 || !wordChar(text.at(at - 1));
        const int end = at + name.size();
        const bool right = end >= text.size() || !wordChar(text.at(end));
        if (left && right)
            return true;
        from = end;
    }
    return false;
}

void noteDefs(const QString &root, const QString &full, const QString &name, QHash<QString, QList<LangDef>> *out)
{
    const QString text = rubyRead(full);
    if (!wordIn(text, name))
        return;
    QHash<QString, QList<LangDef>> local;
    noteRuby(rubyLines(text), QDir(root).relativeFilePath(full), &local);
    const QList<LangDef> found = local.value(name);
    if (!found.isEmpty())
        (*out)[name].append(found);
}

void walkRuby(const QString &root, const QString &dir, const QString &name, QHash<QString, QList<LangDef>> *out, int depth)
{
    if (depth > 24)
        return;
    const QDir folder(dir);
    for (const QFileInfo &info : folder.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (info.isDir()) {
            if (!skipDir(info.fileName()))
                walkRuby(root, info.filePath(), name, out, depth + 1);
            continue;
        }
        if (langFor(info.fileName()) == Lang::Ruby)
            noteDefs(root, info.filePath(), name, out);
    }
}

} // namespace

namespace {

QMutex g_mutex;
QString g_cached;
int g_ticket = 0;
QHash<QString, QList<LangDef>> g_names;
QSet<QString> g_sought;

} // namespace

void rubyWarm(const QString &root, QPointer<QObject> watch, std::function<void()> done)
{
    {
        const QMutexLocker lock(&g_mutex);
        if (g_cached == root)
            return;
        g_cached = root;
        g_names.clear();
        g_sought.clear();
        schemaBump(++g_ticket);
    }
    QObject *target = watch.data();
    if (!target || !done)
        return;
    QMetaObject::invokeMethod(target, [done = std::move(done)] { done(); }, Qt::QueuedConnection);
}

bool rubyReady(const QString &root)
{
    const QMutexLocker lock(&g_mutex);
    return g_cached == root;
}

QList<LangDef> rubyHits(const QString &root, const QString &name)
{
    const QMutexLocker lock(&g_mutex);
    if (g_cached != root)
        return {};
    return g_names.value(name);
}

bool rubySought(const QString &root, const QString &name)
{
    const QMutexLocker lock(&g_mutex);
    return g_cached == root && g_sought.contains(name);
}

void rubyEnsure(const QString &root, const QString &name)
{
    if (root.isEmpty() || name.isEmpty())
        return;
    {
        const QMutexLocker lock(&g_mutex);
        if (g_cached != root || g_sought.contains(name))
            return;
    }
    QHash<QString, QList<LangDef>> found;
    walkRuby(root, root, name, &found, 0);
    const QMutexLocker lock(&g_mutex);
    if (g_cached != root)
        return;
    g_names.insert(name, found.value(name));
    g_sought.insert(name);
}

LangDef rubyPick(const QList<LangDef> &defs, const QString &path)
{
    const QString folder = QFileInfo(path).path();
    LangDef near;
    int count = 0;
    for (const LangDef &def : defs) {
        if (def.path == path)
            return def;
        if (QFileInfo(def.path).path() != folder)
            continue;
        near = def;
        ++count;
    }
    if (count == 1)
        return near;
    if (defs.size() == 1)
        return defs.constFirst();
    return {};
}
