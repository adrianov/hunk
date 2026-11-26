// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangTune.hpp"

#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QMutex>
#include <QSet>
#include <QWaitCondition>

#include <limits>

namespace {

QMutex g_mutex;
QWaitCondition g_done;
QString g_cached;
int g_ticket = 0;
QHash<QString, QList<LangDef>> g_names;
QHash<QString, QList<LangDef>> g_uses;
QSet<QString> g_sought;
QSet<QString> g_pending;
bool g_walking = false;

enum class Seek { Done, Walk, Wait };

void dropNames()
{
    g_names.clear();
    g_uses.clear();
    g_sought.clear();
    g_pending.clear();
    g_walking = false;
    langReset(++g_ticket);
    g_done.wakeAll();
}

Seek beginSeek(const QString &root, const QString &name, int *ticket)
{
    if (g_cached != root || g_sought.contains(name))
        return Seek::Done;
    g_pending.insert(name);
    *ticket = g_ticket;
    if (g_walking)
        return Seek::Wait;
    g_walking = true;
    return Seek::Walk;
}

QStringList takePending(const QString &root, int ticket)
{
    if (g_ticket != ticket || g_cached != root) {
        g_done.wakeAll();
        return {};
    }
    if (g_pending.isEmpty()) {
        g_walking = false;
        g_done.wakeAll();
        return {};
    }
    const QStringList names(g_pending.cbegin(), g_pending.cend());
    g_pending.clear();
    return names;
}

bool storePending(const QString &root, int ticket, const QStringList &names, const QHash<QString, QList<LangDef>> &found,
                  const QHash<QString, QList<LangDef>> &uses)
{
    if (g_ticket != ticket || g_cached != root) {
        g_done.wakeAll();
        return false;
    }
    for (const QString &name : names) {
        g_names.insert(name, found.value(name));
        g_uses.insert(name, uses.value(name));
        g_sought.insert(name);
    }
    g_done.wakeAll();
    return true;
}

QStringList folderParts(const QString &path)
{
    const QString folder = QFileInfo(path).path();
    if (folder.isEmpty() || folder == QLatin1String("."))
        return {};
    return folder.split(QLatin1Char('/'), Qt::SkipEmptyParts);
}

// Longer shared folder wins; among those, fewer extra folders on the definition.
int nearScore(const QString &from, const QString &to)
{
    const QStringList here = folderParts(from);
    const QStringList there = folderParts(to);
    int shared = 0;
    while (shared < here.size() && shared < there.size() && here.at(shared) == there.at(shared))
        ++shared;
    return shared * 1000 - (there.size() - shared);
}

} // namespace

void warmLang(const QString &root, QPointer<QObject> watch, std::function<void()> done)
{
    {
        const QMutexLocker lock(&g_mutex);
        if (g_cached != root) {
            g_cached = root;
            dropNames();
        }
    }
    QObject *target = watch.data();
    if (!target || !done)
        return;
    QMetaObject::invokeMethod(target, [done = std::move(done)] { done(); }, Qt::QueuedConnection);
}

void forgetLang()
{
    const QMutexLocker lock(&g_mutex);
    dropNames();
}

bool langCached(const QString &root)
{
    const QMutexLocker lock(&g_mutex);
    return g_cached == root;
}

QList<LangDef> langHits(const QString &root, const QString &name, Lang lang)
{
    const QMutexLocker lock(&g_mutex);
    if (g_cached != root)
        return {};
    QList<LangDef> hits;
    for (const LangDef &def : g_names.value(name)) {
        if (langFor(def.path) == lang)
            hits.append(def);
    }
    return hits;
}

QList<LangDef> langUses(const QString &root, const QString &name)
{
    const QMutexLocker lock(&g_mutex);
    if (g_cached != root)
        return {};
    return g_uses.value(name);
}

bool langSought(const QString &root, const QString &name)
{
    const QMutexLocker lock(&g_mutex);
    return g_cached == root && g_sought.contains(name);
}

void waitSought(const QString &root, const QString &name, int ticket)
{
    const QMutexLocker lock(&g_mutex);
    while (g_ticket == ticket && g_cached == root && !g_sought.contains(name))
        g_done.wait(&g_mutex);
}

void drain(const QString &root, int ticket)
{
    while (true) {
        QStringList names;
        {
            const QMutexLocker lock(&g_mutex);
            names = takePending(root, ticket);
        }
        if (names.isEmpty())
            return;
        QHash<QString, QList<LangDef>> found;
        QHash<QString, QList<LangDef>> uses;
        langWalk(root, names, &found, &uses);
        const QMutexLocker lock(&g_mutex);
        if (!storePending(root, ticket, names, found, uses))
            return;
    }
}

// One walk drains every name queued for this root. A root change drops that batch.
void langEnsure(const QString &root, const QString &name)
{
    if (root.isEmpty() || name.isEmpty())
        return;
    int ticket = 0;
    Seek role = Seek::Done;
    {
        const QMutexLocker lock(&g_mutex);
        role = beginSeek(root, name, &ticket);
    }
    if (role == Seek::Walk)
        drain(root, ticket);
    else if (role == Seek::Wait)
        waitSought(root, name, ticket);
}

LangDef langPick(const QList<LangDef> &defs, const QString &path)
{
    if (defs.isEmpty())
        return {};
    if (defs.size() == 1)
        return defs.constFirst();
    LangDef best;
    int bestScore = std::numeric_limits<int>::min();
    int ties = 0;
    for (const LangDef &def : defs) {
        if (def.path == path)
            return def;
        const int score = nearScore(path, def.path);
        if (score < bestScore)
            continue;
        if (score == bestScore) {
            ++ties;
            continue;
        }
        best = def;
        bestScore = score;
        ties = 1;
    }
    return ties == 1 ? best : LangDef();
}
