// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QMutex>

namespace {

QMutex g_mix;
QHash<QString, QStringList> g_hosts;

bool wordEdge(const QString &line, int end)
{
    return end >= line.size() || (!line.at(end).isLetterOrNumber() && line.at(end) != QLatin1Char('_'));
}

bool headKey(const QString &line, QLatin1String key, int *at)
{
    const int cursor = rubySkip(line, 0);
    if (!line.mid(cursor).startsWith(key) || !wordEdge(line, cursor + int(key.size())))
        return false;
    *at = rubySkip(line, cursor + int(key.size()));
    return *at < line.size() && line.at(*at).isUpper();
}

int constBack(const QString &line, int at)
{
    while (at > 0 && (rubyWordChar(line.at(at - 1)) || line.at(at - 1) == QLatin1Char(':')))
        --at;
    return at;
}

int mixAt(const QString &line, QLatin1String key, int *paren)
{
    const QString token = QLatin1Char('.') + key;
    for (int from = 0; from < line.size();) {
        const int at = line.indexOf(token, from);
        if (at <= 0)
            return -1;
        const int after = at + token.size();
        const int open = rubySkip(line, after);
        if (wordEdge(line, after) && open < line.size() && line.at(open) == QLatin1Char('(')) {
            *paren = open;
            return at;
        }
        from = after;
    }
    return -1;
}

bool mixCall(const QString &line, QLatin1String key, QString *module, QString *host)
{
    int paren = 0;
    const int at = mixAt(line, key, &paren);
    if (at < 0)
        return false;
    const QString type = rubyConst(line, constBack(line, at), at);
    if (type.isEmpty())
        return false;
    const int arg = rubySkip(line, paren + 1);
    const QString mod = rubyConst(line, arg, rubyWordEnd(line, arg));
    if (mod.isEmpty())
        return false;
    *module = mod;
    *host = type;
    return true;
}

} // namespace

bool rubyMixin(const QString &line, QString *name)
{
    int at = 0;
    if (!headKey(line, QLatin1String("prepend"), &at) && !headKey(line, QLatin1String("include"), &at))
        return false;
    int end = at;
    while (end < line.size() && (line.at(end).isLetterOrNumber() || line.at(end) == QLatin1Char('_')))
        ++end;
    const QString path = rubyConst(line, at, end);
    if (path.isEmpty())
        return false;
    *name = path;
    return true;
}

bool rubyCallMix(const QString &line, QString *module, QString *host)
{
    return mixCall(line, QLatin1String("prepend"), module, host)
        || mixCall(line, QLatin1String("include"), module, host);
}

void rubyResetMix()
{
    const QMutexLocker lock(&g_mix);
    g_hosts.clear();
}

void rubyAddHost(const QString &module, const QString &host)
{
    if (module.isEmpty() || host.isEmpty())
        return;
    const QMutexLocker lock(&g_mix);
    QStringList &list = g_hosts[module];
    if (!list.contains(host))
        list.append(host);
}

QStringList rubyHosts(const QString &module)
{
    const QMutexLocker lock(&g_mix);
    return g_hosts.value(module);
}

QStringList rubyMixed(const QString &tail)
{
    if (tail.isEmpty())
        return {};
    const QString suffix = QStringLiteral("::") + tail;
    QStringList mods;
    const QMutexLocker lock(&g_mix);
    for (auto it = g_hosts.cbegin(); it != g_hosts.cend(); ++it) {
        for (const QString &host : it.value()) {
            if ((host == tail || host.endsWith(suffix)) && !mods.contains(it.key()))
                mods.append(it.key());
        }
    }
    return mods;
}
