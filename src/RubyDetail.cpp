// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>

namespace {

QMutex g_mutex;

struct CachedFile {
    qint64 mtime = -1;
    QStringList lines;
};

constexpr int kLineKeep = 8;

QHash<QString, CachedFile> g_files;

void storeLines(const QString &key, qint64 mtime, const QStringList &lines)
{
    CachedFile cached;
    cached.mtime = mtime;
    cached.lines = lines;
    const QMutexLocker lock(&g_mutex);
    if (!g_files.contains(key) && g_files.size() >= kLineKeep)
        g_files.erase(g_files.begin());
    g_files.insert(key, cached);
}

QString fileKey(const QString &root, const QString &path)
{
    return root + QLatin1Char('\n') + path;
}

qint64 fileMtime(const QString &root, const QString &path)
{
    return QFileInfo(QDir(root).filePath(path)).lastModified().toMSecsSinceEpoch();
}

} // namespace

QStringList linesFor(const QString &root, const QString &path)
{
    const QString key = fileKey(root, path);
    const qint64 mtime = fileMtime(root, path);
    {
        const QMutexLocker lock(&g_mutex);
        const auto found = g_files.constFind(key);
        if (found != g_files.cend() && found->mtime == mtime)
            return found->lines;
    }
    const QStringList lines = readLines(root, path);
    storeLines(key, mtime, lines);
    return lines;
}

void rubyForgetLines()
{
    const QMutexLocker lock(&g_mutex);
    g_files.clear();
}

QStringList readLines(const QString &root, const QString &path)
{
    QFile file(QDir(root).filePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return rubyLines(QString::fromUtf8(file.readAll()));
}

int defLine(const QStringList &lines, int line)
{
    int limit = rubyIndent(lines.at(line - 1));
    for (int at = line; at >= 1; --at) {
        const QString text = lines.at(at - 1);
        if (text.isEmpty() || rubyIndent(text) > limit)
            continue;
        limit = rubyIndent(text);
        QString name;
        if (rubyDefName(text, &name))
            return at;
        if (rubyTypeName(text, &name))
            return 0;
    }
    return 0;
}

int methodLast(const QStringList &lines, int opened)
{
    if (rubyClosed(lines.at(opened - 1)))
        return opened;
    const int indent = rubyIndent(lines.at(opened - 1));
    for (int at = opened + 1; at <= lines.size(); ++at) {
        if (rubyIndent(lines.at(at - 1)) <= indent && rubyEndLine(lines.at(at - 1)))
            return at;
    }
    return lines.size();
}

bool defSig(const LangDef &def)
{
    return def.signature.trimmed().startsWith(QLatin1String("def "));
}

QString constBefore(const QString &text, int dot)
{
    int at = dot;
    while (at > 0 && rubyWordChar(text.at(at - 1)))
        --at;
    return rubyConst(text, at, dot);
}

bool keepOwner(QString *found, const QString &owner)
{
    if (owner.isEmpty())
        return true;
    if (!found->isEmpty() && *found != owner)
        return false;
    *found = owner;
    return true;
}

bool plainName(const QString &name)
{
    return !name.isEmpty() && name.at(0).isLower() && name.indexOf('.') < 0 && name.indexOf(':') < 0;
}

RubyBound rubyBound(const LangSpot &spot)
{
    RubyBound bound;
    if (spot.line < 1)
        return bound;
    bound.lines = linesFor(spot.root, spot.path);
    if (spot.line > bound.lines.size())
        return bound;
    bound.opened = defLine(bound.lines, spot.line);
    if (bound.opened <= 0)
        return bound;
    bound.last = methodLast(bound.lines, bound.opened);
    return bound;
}
