// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "RubyDetail.hpp"

namespace {

struct Scope {
    int indent = 0;
    int line = 0;
    QString name;
};

void popScope(const QString &line, QList<Scope> *stack)
{
    if (!rubyEndLine(line))
        return;
    while (!stack->isEmpty() && rubyIndent(line) <= stack->constLast().indent)
        stack->removeLast();
}

QString nestName(const QList<Scope> &stack, const QString &name)
{
    if (name.contains(QLatin1String("::")) || stack.isEmpty())
        return name;
    return stack.constLast().name + QStringLiteral("::") + name;
}

void pushScope(const QString &text, int line, const QString &name, QList<Scope> *stack)
{
    Scope scope;
    scope.indent = rubyIndent(text);
    scope.line = line;
    scope.name = nestName(*stack, name);
    stack->append(scope);
}

QStringList parentOpens(const QStringList &lines, const QList<Scope> &stack)
{
    QStringList opens;
    for (int at = 0; at < stack.size() - 1; ++at) {
        const int line = stack.at(at).line;
        if (line >= 1 && line <= lines.size())
            opens.append(lines.at(line - 1));
    }
    return opens;
}

void noteOwned(const QStringList &lines, int line, const QString &path, const QString &name, const QString &owner,
               QHash<QString, QList<LangDef>> *out)
{
    LangDef def = rubyAt(lines, line, path);
    def.owner = owner;
    (*out)[name].push_back(def);
}

QString classOwner(const QList<Scope> &stack)
{
    for (int at = stack.size() - 1; at >= 0; --at) {
        if (!stack.at(at).name.isEmpty())
            return stack.at(at).name;
    }
    return {};
}

bool inMethod(const QList<Scope> &stack)
{
    return !stack.isEmpty() && stack.constLast().name.isEmpty();
}

void pushMethod(const QString &line, QList<Scope> *stack)
{
    if (rubyClosed(line))
        return;
    Scope method;
    method.indent = rubyIndent(line);
    stack->append(method);
}

void noteMetric(const QStringList &lines, int line, const QString &path, const QString &name, const QString &owner,
                QHash<QString, QList<LangDef>> *out)
{
    LangDef def;
    def.path = path;
    def.line = line;
    def.signature = rubyJoin(lines, line, rubySigEnd(lines, line));
    def.owner = owner;
    (*out)[name].push_back(def);
}

bool noteScope(const QStringList &lines, int line, const QString &path, const QString &text, QList<Scope> *stack,
               QHash<QString, QList<LangDef>> *out)
{
    QString name;
    if (rubyDefName(text, &name)) {
        noteOwned(lines, line, path, name, classOwner(*stack), out);
        pushMethod(text, stack);
        return true;
    }
    if (!rubyTypeName(text, &name))
        return false;
    pushScope(text, line, name, stack);
    const QString full = stack->constLast().name;
    noteOwned(lines, line, path, full, {}, out);
    rubyWrap(&(*out)[full].last(), parentOpens(lines, *stack));
    if (full != name)
        (*out)[name].append((*out)[full].constLast());
    return true;
}

bool noteValue(const QStringList &lines, int line, const QString &path, const QString &text, const QList<Scope> &stack,
               QHash<QString, QList<LangDef>> *out)
{
    QString name;
    if (rubyAssign(text, &name)) {
        noteOwned(lines, line, path, name, {}, out);
        (*out)[name].last().body.clear();
        return true;
    }
    const QString owner = classOwner(stack);
    if (owner.isEmpty() || inMethod(stack) || !rubyDsl(text, &name))
        return false;
    noteMetric(lines, line, path, name, owner, out);
    return true;
}

void noteMixin(const QString &text, const QList<Scope> &stack)
{
    QString name;
    if (inMethod(stack) || !rubyMixin(text, &name))
        return;
    const QString host = classOwner(stack);
    if (!host.isEmpty())
        rubyAddHost(name, host);
}

void scanLine(const QStringList &lines, int line, const QString &path, QList<Scope> *stack, QHash<QString, QList<LangDef>> *out)
{
    const QString text = lines.at(line - 1);
    popScope(text, stack);
    if (!noteScope(lines, line, path, text, stack, out))
        noteValue(lines, line, path, text, *stack, out);
    noteMixin(text, *stack);
}

} // namespace

void noteRuby(const QStringList &lines, const QString &path, QHash<QString, QList<LangDef>> *out)
{
    QList<Scope> stack;
    for (int line = 1; line <= lines.size(); ++line)
        scanLine(lines, line, path, &stack, out);
}
