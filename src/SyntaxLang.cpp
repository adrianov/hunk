#include "Syntax.hpp"

#include "SyntaxRule.hpp"

namespace {

const char *kRuby[] = {
    "alias", "and", "begin", "break", "case", "class", "def", "defined?", "do", "else", "elsif", "end",
    "ensure", "false", "for", "if", "in", "module", "next", "nil", "not", "or", "redo", "rescue", "retry",
    "return", "self", "super", "then", "true", "undef", "unless", "until", "when", "while", "yield",
    "require", "include", "extend", "raise", "attr_reader", "attr_writer", "attr_accessor", "private",
    "protected", "public", nullptr};
const char *kPy[] = {
    "False", "None", "True", "and", "as", "assert", "async", "await", "break", "class", "continue", "def",
    "del", "elif", "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield", nullptr};
const char *kJs[] = {
    "async", "await", "break", "case", "catch", "class", "const", "continue", "debugger", "default", "delete",
    "do", "else", "export", "extends", "false", "finally", "for", "function", "if", "import", "in",
    "instanceof", "let", "new", "null", "of", "return", "static", "super", "switch", "this", "throw", "true",
    "try", "typeof", "undefined", "var", "void", "while", "yield", nullptr};
const char *kCpp[] = {
    "auto", "bool", "break", "case", "catch", "char", "class", "const", "constexpr", "continue", "delete",
    "do", "else", "enum", "false", "for", "if", "int", "namespace", "new", "nullptr", "override", "private",
    "protected", "public", "return", "sizeof", "static", "struct", "switch", "template", "this", "throw",
    "true", "try", "typedef", "typename", "using", "virtual", "void", "while", nullptr};
const char *kRust[] = {
    "as", "async", "await", "break", "const", "continue", "crate", "dyn", "else", "enum", "false", "fn",
    "for", "if", "impl", "in", "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return", "self",
    "struct", "super", "trait", "true", "type", "unsafe", "use", "where", "while", nullptr};
const char *kGo[] = {
    "break", "case", "chan", "const", "continue", "defer", "else", "fallthrough", "false", "for", "func",
    "go", "goto", "if", "import", "interface", "map", "nil", "package", "range", "return", "select",
    "struct", "switch", "true", "type", "var", nullptr};
const char *kJava[] = {
    "boolean", "break", "case", "catch", "class", "continue", "else", "enum", "extends", "false", "final",
    "for", "if", "implements", "import", "instanceof", "int", "interface", "new", "null", "package",
    "private", "protected", "public", "return", "static", "super", "switch", "this", "throw", "throws",
    "true", "try", "void", "while", nullptr};
const char *kKotlin[] = {
    "as", "break", "catch", "class", "companion", "continue", "data", "else", "false", "for", "fun", "if",
    "import", "in", "interface", "internal", "is", "null", "object", "override", "package", "private",
    "protected", "public", "return", "sealed", "super", "suspend", "this", "throw", "true", "try", "val",
    "var", "when", "while", nullptr};
const char *kSwift[] = {
    "as", "break", "case", "catch", "class", "continue", "defer", "else", "enum", "extension", "false",
    "for", "func", "guard", "if", "import", "in", "inout", "internal", "let", "nil", "private", "protocol",
    "public", "return", "self", "static", "struct", "super", "switch", "throw", "throws", "true", "try",
    "var", "where", "while", nullptr};
const char *kShell[] = {
    "case", "do", "done", "echo", "elif", "else", "esac", "exit", "export", "false", "fi", "for", "function",
    "if", "in", "local", "return", "then", "true", "while", nullptr};
const char *kPhp[] = {
    "as", "break", "case", "catch", "class", "const", "continue", "echo", "else", "elseif", "extends",
    "false", "for", "foreach", "function", "if", "implements", "interface", "namespace", "new", "null",
    "private", "protected", "public", "return", "static", "switch", "throw", "true", "try", "use",
    "while", nullptr};
const char *kLua[] = {
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "if", "in", "local", "nil",
    "not", "or", "repeat", "return", "then", "true", "until", "while", nullptr};
const char *kElixir[] = {
    "alias", "and", "case", "cond", "def", "defmodule", "defp", "do", "else", "end", "false", "fn", "for",
    "if", "import", "nil", "not", "or", "raise", "require", "true", "unless", "use", "when", "with", nullptr};
const char *kSql[] = {
    "and", "as", "by", "create", "delete", "distinct", "false", "from", "group", "having", "inner", "insert",
    "into", "join", "left", "limit", "null", "offset", "on", "or", "order", "outer", "right", "select",
    "set", "table", "true", "union", "update", "values", "where", nullptr};
const char *kJson[] = {"true", "false", "null", nullptr};
const char *kYaml[] = {"true", "false", "yes", "no", "null", nullptr};
const char *kCss[] = {"important", nullptr};

struct Ext {
    const char *ext;
    Lang lang;
};

const Ext kExts[] = {
    {"rb", Lang::Ruby}, {"rake", Lang::Ruby}, {"ru", Lang::Ruby}, {"gemspec", Lang::Ruby},
    {"gemfile", Lang::Ruby}, {"rakefile", Lang::Ruby}, {"brewfile", Lang::Ruby}, {"vagrantfile", Lang::Ruby},
    {"py", Lang::Py}, {"pyi", Lang::Py}, {"pyw", Lang::Py},
    {"js", Lang::Js}, {"jsx", Lang::Js}, {"mjs", Lang::Js}, {"cjs", Lang::Js}, {"ts", Lang::Js}, {"tsx", Lang::Js},
    {"c", Lang::Cpp}, {"h", Lang::Cpp}, {"hh", Lang::Cpp}, {"hpp", Lang::Cpp}, {"cc", Lang::Cpp},
    {"cpp", Lang::Cpp}, {"cxx", Lang::Cpp}, {"m", Lang::Cpp}, {"mm", Lang::Cpp},
    {"rs", Lang::Rust}, {"go", Lang::Go}, {"java", Lang::Java}, {"kt", Lang::Kotlin}, {"kts", Lang::Kotlin},
    {"swift", Lang::Swift},
    {"sh", Lang::Shell}, {"bash", Lang::Shell}, {"zsh", Lang::Shell}, {"dockerfile", Lang::Shell}, {"makefile", Lang::Shell},
    {"css", Lang::Css}, {"scss", Lang::Css},
    {"html", Lang::Html}, {"htm", Lang::Html}, {"xml", Lang::Html}, {"svg", Lang::Html}, {"vue", Lang::Html},
    {"json", Lang::Json}, {"yml", Lang::Yaml}, {"yaml", Lang::Yaml},
    {"sql", Lang::Sql}, {"php", Lang::Php}, {"lua", Lang::Lua}, {"ex", Lang::Elixir}, {"exs", Lang::Elixir},
};

QString baseName(const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    return (slash < 0 ? path : path.mid(slash + 1)).toLower();
}

QString extOf(const QString &base)
{
    const int dot = base.lastIndexOf(QLatin1Char('.'));
    return dot < 0 ? base : base.mid(dot + 1);
}

Rule scriptRule(Lang lang)
{
    switch (lang) {
    case Lang::Ruby:
        return {"#", "=begin", "=end", kRuby, RubyId | Marks | Caps};
    case Lang::Py:
        return {"#", nullptr, nullptr, kPy, Triple | Caps};
    case Lang::Js:
        return {"//", "/*", "*/", kJs, Caps};
    case Lang::Shell:
        return {"#", nullptr, nullptr, kShell, Marks};
    case Lang::Php:
        return {"//", "/*", "*/", kPhp, Marks | Caps};
    case Lang::Lua:
        return {"--", "--[[", "]]", kLua, 0};
    case Lang::Elixir:
        return {"#", nullptr, nullptr, kElixir, Marks | Caps};
    case Lang::Json:
        return {nullptr, nullptr, nullptr, kJson, 0};
    case Lang::Yaml:
        return {"#", nullptr, nullptr, kYaml, 0};
    default:
        return {};
    }
}

Rule systemRule(Lang lang)
{
    switch (lang) {
    case Lang::Cpp:
        return {"//", "/*", "*/", kCpp, Preproc | Caps};
    case Lang::Rust:
        return {"//", "/*", "*/", kRust, Caps};
    case Lang::Go:
        return {"//", "/*", "*/", kGo, Caps};
    case Lang::Java:
        return {"//", "/*", "*/", kJava, Caps};
    case Lang::Kotlin:
        return {"//", "/*", "*/", kKotlin, Caps};
    case Lang::Swift:
        return {"//", "/*", "*/", kSwift, Caps};
    case Lang::Css:
        return {nullptr, "/*", "*/", kCss, Css};
    case Lang::Html:
        return {nullptr, "<!--", "-->", nullptr, Html};
    case Lang::Sql:
        return {"--", "/*", "*/", kSql, Fold};
    default:
        return {};
    }
}

bool active(const Rule &rule)
{
    return rule.line || rule.open || rule.words || rule.flags != 0;
}

} // namespace

Lang langFor(const QString &path)
{
    const QString ext = extOf(baseName(path));
    for (const Ext &item : kExts) {
        if (ext == QLatin1String(item.ext))
            return item.lang;
    }
    return Lang::None;
}

Rule ruleFor(Lang lang)
{
    const Rule script = scriptRule(lang);
    if (active(script))
        return script;
    return systemRule(lang);
}

QString sidePath(const FileDiff &file, bool left)
{
    const QString path = left ? file.oldPath : file.newPath;
    if (path.isEmpty() || path == QLatin1String("/dev/null"))
        return file.path();
    return path;
}

void colorPath(FileDiff *file, bool left)
{
    const Rule rule = ruleFor(langFor(sidePath(*file, left)));
    if (active(rule))
        colorSide(file, left, rule);
}

void highlightDoc(DiffDoc *doc)
{
    for (FileDiff &file : doc->files) {
        if (file.binary)
            continue;
        colorPath(&file, true);
        colorPath(&file, false);
    }
}
