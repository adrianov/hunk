// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

#include <QHash>
#include <QMutex>

namespace {

QMutex g_mutex;
int g_ticket = 0;
QString g_root;
QHash<QString, LangDef> g_columns;

bool yPlural(const QString &name, QString *out)
{
    if (name.size() < 2 || !name.endsWith(QLatin1Char('y')))
        return false;
    if (QStringLiteral("aeiou").contains(name.at(name.size() - 2).toLower()))
        return false;
    *out = name.chopped(1) + QStringLiteral("ies");
    return true;
}

bool esEnding(const QString &name)
{
    return name.endsWith(QLatin1String("ch")) || name.endsWith(QLatin1String("sh")) || name.endsWith(QLatin1Char('x'))
        || name.endsWith(QLatin1Char('z'));
}

QString plural(const QString &name)
{
    QString ies;
    if (yPlural(name, &ies))
        return ies;
    const bool es = name.endsWith(QLatin1Char('s')) || esEnding(name);
    return name + (es ? QStringLiteral("es") : QStringLiteral("s"));
}

LangDef stored(const QString &root, const QString &table, const QString &column)
{
    const QMutexLocker lock(&g_mutex);
    if (g_root != root || table.isEmpty())
        return {};
    return g_columns.value(table + QLatin1Char('\n') + column);
}

} // namespace

void schemaBump(int ticket)
{
    const QMutexLocker lock(&g_mutex);
    g_ticket = ticket;
}

void schemaStore(int ticket, const QString &root, QHash<QString, LangDef> columns)
{
    const QMutexLocker lock(&g_mutex);
    if (ticket != g_ticket)
        return;
    g_root = root;
    g_columns = std::move(columns);
}

void schemaLazy(const QString &root)
{
    int ticket = 0;
    {
        const QMutexLocker lock(&g_mutex);
        if (g_root == root)
            return;
        ticket = g_ticket;
    }
    QHash<QString, LangDef> columns;
    readSchemas(root, &columns);
    schemaStore(ticket, root, std::move(columns));
}

LangDef schemaFind(const QString &root, const QString &receiver, const QString &column)
{
    QString name = receiver;
    while (name.startsWith(QLatin1Char('@')))
        name.remove(0, 1);
    if (name.isEmpty() || !name.at(0).isLower())
        return {};
    schemaLazy(root);
    const LangDef exact = stored(root, name, column);
    if (exact.found())
        return exact;
    return stored(root, plural(name), column);
}
