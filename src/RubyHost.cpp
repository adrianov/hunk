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
