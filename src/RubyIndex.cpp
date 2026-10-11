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

void noteDefs(const QString &root, const QString &full, QHash<QString, QList<LangDef>> *out)
{
    const QStringList lines = rubyLines(rubyRead(full));
    const QString path = QDir(root).relativeFilePath(full);
    for (int line = 1; line <= lines.size(); ++line) {
        QString name;
        if (!rubyDefName(lines.at(line - 1), &name))
            continue;
        (*out)[name].push_back(rubyAt(lines, line, path));
    }
}

void walkRuby(const QString &root, const QString &dir, QHash<QString, QList<LangDef>> *out, int depth)
{
    if (depth > 24)
        return;
    const QDir folder(dir);
    for (const QFileInfo &info : folder.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (info.isDir()) {
            if (!skipDir(info.fileName()))
                walkRuby(root, info.filePath(), out, depth + 1);
            continue;
        }
        if (langFor(info.fileName()) == Lang::Ruby)
            noteDefs(root, info.filePath(), out);
    }
}

} // namespace

namespace {

QMutex g_mutex;
QString g_cached;
int g_ticket = 0;
QHash<QString, QList<LangDef>> g_names;

} // namespace

void rubyWarm(const QString &root, QPointer<QObject> watch, std::function<void()> done)
{
    int ticket = 0;
    {
        const QMutexLocker lock(&g_mutex);
        if (g_cached == root)
            return;
        ticket = ++g_ticket;
    }
    QHash<QString, QList<LangDef>> names;
    if (!root.isEmpty())
        walkRuby(root, root, &names, 0);
    {
        const QMutexLocker lock(&g_mutex);
        if (ticket != g_ticket)
            return;
        g_cached = root;
        g_names = std::move(names);
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
