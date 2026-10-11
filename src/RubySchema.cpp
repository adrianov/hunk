// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

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

bool nameChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool quotedName(const QString &line, int at, QString *name)
{
    if (at >= line.size() || line.at(at) != QLatin1Char('"'))
        return false;
    const int end = line.indexOf(QLatin1Char('"'), at + 1);
    if (end <= at + 1)
        return false;
    *name = line.mid(at + 1, end - at - 1);
    return true;
}

int wordEnd(const QString &line, int at)
{
    while (at < line.size() && nameChar(line.at(at)))
        ++at;
    return at;
}

bool symbolName(const QString &line, int at, QString *name)
{
    if (at + 1 >= line.size() || line.at(at) != QLatin1Char(':') || !line.at(at + 1).isLetter())
        return false;
    *name = line.mid(at + 1, wordEnd(line, at + 1) - at - 1);
    return true;
}

bool takeName(const QString &line, int at, QString *name)
{
    return quotedName(line, at, name) || symbolName(line, at, name);
}

bool createTable(const QString &line, QString *table)
{
    const int at = line.indexOf(QLatin1String("create_table"));
    if (at < 0 || (at > 0 && nameChar(line.at(at - 1))))
        return false;
    return takeName(line, rubySkip(line, at + 12), table);
}

bool columnName(const QString &line, QString *name)
{
    const int dot = line.indexOf(QLatin1Char('.'));
    if (dot < 1 || dot + 1 >= line.size() || !line.at(dot + 1).isLetter())
        return false;
    int at = dot + 1;
    while (at < line.size() && nameChar(line.at(at)))
        ++at;
    return takeName(line, rubySkip(line, at), name);
}

QString columnKey(const QString &table, const QString &name)
{
    return table + QLatin1Char('\n') + name;
}

void noteColumn(const QString &text, const QString &table, const QString &path, int number, QHash<QString, LangDef> *out)
{
    QString name;
    if (!columnName(text, &name) || out->contains(columnKey(table, name)))
        return;
    LangDef def;
    def.path = path;
    def.line = number;
    def.comment = QStringLiteral("# ") + table;
    def.signature = text.trimmed();
    (*out)[columnKey(table, name)] = def;
}

void applyLine(const QString &text, QString *table, const QString &path, int number, QHash<QString, LangDef> *out)
{
    if (text.trimmed().startsWith(QLatin1Char('#')))
        return;
    QString name;
    if (createTable(text, &name)) {
        *table = name;
        return;
    }
    if (table->isEmpty())
        return;
    if (text.trimmed() == QLatin1String("end")) {
        table->clear();
        return;
    }
    noteColumn(text, *table, path, number, out);
}

void readSchema(const QString &root, const QString &full, QHash<QString, LangDef> *out)
{
    const QStringList lines = rubyLines(readFile(full));
    const QString path = QDir(root).relativeFilePath(full);
    QString table;
    for (int line = 1; line <= lines.size(); ++line)
        applyLine(lines.at(line - 1), &table, path, line, out);
}

} // namespace

void readSchemas(const QString &root, QHash<QString, LangDef> *out)
{
    const QDir dir(QDir(root).filePath(QStringLiteral("db")));
    readSchema(root, dir.filePath(QStringLiteral("schema.rb")), out);
    for (const QFileInfo &info : dir.entryInfoList({QStringLiteral("*_schema.rb")}, QDir::Files))
        readSchema(root, info.filePath(), out);
}
