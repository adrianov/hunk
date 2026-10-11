// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace {

int g_fails = 0;

void check(bool ok, const char *text, int line)
{
    if (ok)
        return;
    qCritical() << "fail" << text << "at" << line;
    ++g_fails;
}

#define CHECK(cond) check(static_cast<bool>(cond), #cond, __LINE__)

bool writeAt(const QString &path, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(text.toUtf8()) >= 0;
}

LangDef lookupName(const QString &root, const QString &path, const QString &name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = path;
    spot.name = name;
    return lookupDef(spot);
}

void testRubyBody(const QString &root)
{
    const LangDef def = lookupName(root, QStringLiteral("app.rb"), QStringLiteral("counter"));
    CHECK(def.path == QLatin1String("app.rb") && def.line == 5);
    CHECK(def.comment.contains(QLatin1String("Counts widgets")));
    CHECK(def.signature == QLatin1String("def counter(start)"));
}

void testRubyTail(const QString &root)
{
    const LangDef def = lookupName(root, QStringLiteral("app.rb"), QStringLiteral("counter"));
    CHECK(def.body.contains(QLatin1String("start + 1")));
    CHECK(def.body.trimmed().endsWith(QLatin1String("end")));
}

void testRubySelf(const QString &root)
{
    const LangDef def = lookupName(root, QStringLiteral("app.rb"), QStringLiteral("build"));
    CHECK(def.line == 9);
    CHECK(def.signature.contains(QLatin1String("def self.build")));
    CHECK(def.body.contains(QLatin1String("if ready")));
    CHECK(def.body.trimmed().endsWith(QLatin1String("end")));
}

void testRubyOther(const QString &root)
{
    const LangDef def = lookupName(root, QStringLiteral("call.rb"), QStringLiteral("counter"));
    CHECK(def.path == QLatin1String("app.rb"));
    CHECK(def.line == 5);
}

void testRubyHere(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app.rb");
    spot.name = QStringLiteral("counter");
    spot.line = 5;
    CHECK(!lookupDef(spot).found());
    spot.line = 6;
    CHECK(lookupDef(spot).line == 5);
}

void testRubyMiss(const QString &root)
{
    CHECK(!lookupName(root, QStringLiteral("app.rb"), QStringLiteral("missing")).found());
    CHECK(!lookupName(root, QStringLiteral("app.cpp"), QStringLiteral("counter")).found());
}

} // namespace

int rubyPickTests();

QString sampleRuby()
{
    return QStringLiteral("def other\n"
                          "end\n"
                          "\n"
                          "# Counts widgets\n"
                          "def counter(start)\n"
                          "  start + 1\n"
                          "end\n"
                          "\n"
                          "def self.build\n"
                          "  if ready\n"
                          "    1\n"
                          "  end\n"
                          "  2\n"
                          "end\n");
}

void writeSample(const QString &root)
{
    writeAt(QDir(root).filePath(QStringLiteral("app.rb")), sampleRuby());
    writeAt(QDir(root).filePath(QStringLiteral("call.rb")), QStringLiteral("counter\n"));
    writeAt(QDir(root).filePath(QStringLiteral("app.cpp")), QStringLiteral("void counter() {}\n"));
}

void rubySample(const QString &root)
{
    warmLang(root);
    testRubyBody(root);
    testRubyTail(root);
    testRubySelf(root);
    testRubyOther(root);
    testRubyHere(root);
    testRubyMiss(root);
}

int rubyTests()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    writeSample(dir.path());
    rubySample(dir.path());
    return g_fails + rubyPickTests();
}
