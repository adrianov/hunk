// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

int indentOf(const QString &line)
{
    int col = 0;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' '))
            ++col;
        else if (ch == QLatin1Char('\t'))
            col += 4;
        else
            break;
    }
    return col;
}

bool wordEdge(const QString &line, int at, int length)
{
    const bool left = at == 0 || (!line.at(at - 1).isLetterOrNumber() && line.at(at - 1) != QLatin1Char('_'));
    const int after = at + length;
    const bool right = after >= line.size() || (!line.at(after).isLetterOrNumber() && line.at(after) != QLatin1Char('_'));
    return left && right;
}

bool isEnd(const QString &line)
{
    const int at = rubySkip(line, 0);
    return line.mid(at).startsWith(QLatin1String("end")) && wordEdge(line, at, 3);
}

bool quoteChar(QChar ch)
{
    return ch == QLatin1Char('"') || ch == QLatin1Char('\'');
}

int skipQuote(const QString &line, int at)
{
    const QChar quote = line.at(at);
    for (++at; at < line.size(); ++at) {
        if (line.at(at) == QLatin1Char('\\'))
            ++at;
        else if (line.at(at) == quote)
            return at + 1;
    }
    return line.size();
}

bool endlessMark(const QString &line, int index)
{
    if (line.at(index) != QLatin1Char('='))
        return false;
    const QChar prev = index > 0 ? line.at(index - 1) : QLatin1Char(' ');
    if (!prev.isSpace() && prev != QLatin1Char(')'))
        return false;
    return index + 1 >= line.size() || line.at(index + 1) != QLatin1Char('=');
}

bool closedLine(const QString &line)
{
    for (int at = 0; at < line.size();) {
        const QChar ch = line.at(at);
        if (quoteChar(ch)) {
            at = skipQuote(line, at);
            continue;
        }
        if (ch == QLatin1Char('#'))
            return false;
        if (line.mid(at).startsWith(QLatin1String("end")) && wordEdge(line, at, 3))
            return true;
        ++at;
    }
    return false;
}

bool endlessDef(const QString &line)
{
    int depth = 0;
    for (int index = 0; index < line.size();) {
        const QChar ch = line.at(index);
        if (quoteChar(ch)) {
            index = skipQuote(line, index);
            continue;
        }
        if (ch == QLatin1Char('('))
            ++depth;
        else if (ch == QLatin1Char(')') && depth > 0)
            --depth;
        else if (depth == 0 && endlessMark(line, index))
            return true;
        ++index;
    }
    return false;
}

bool stopBody(const QString &text, int indent)
{
    return indentOf(text) <= indent && isEnd(text);
}

QString takeBody(const QStringList &lines, int indent, int sigEnd)
{
    QStringList body;
    for (int at = sigEnd + 1; at <= lines.size(); ++at) {
        body.append(lines.at(at - 1));
        if (stopBody(lines.at(at - 1), indent))
            break;
    }
    return body.join(QLatin1Char('\n'));
}

} // namespace

bool rubyClosed(const QString &line)
{
    return endlessDef(line) || closedLine(line);
}

LangDef rubyAt(const QStringList &lines, int line, const QString &path)
{
    const int sigEnd = rubySigEnd(lines, line);
    LangDef def;
    def.path = path;
    def.line = line;
    def.comment = rubyComment(lines, line);
    def.signature = rubyJoin(lines, line, sigEnd);
    def.body = endlessDef(lines.at(line - 1)) || closedLine(lines.at(line - 1))
                   ? QString()
                   : takeBody(lines, indentOf(lines.at(line - 1)), sigEnd);
    return def;
}
