// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "RubyDetail.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

LangDef lookupName(const QString &root, const char *path, const char *name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.name = QLatin1String(name);
    return lookupDef(spot);
}

bool putFile(const QString &root, const char *rel, const char *text)
{
    const QString path = QDir(root).filePath(QLatin1String(rel));
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(QLatin1String(text).toUtf8()) >= 0;
}

void testRubyNear()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "a/one.rb", "def near\n  1\nend\n") && putFile(root, "b/two.rb", "def near\n  2\nend\n")
        && putFile(root, "a/call.rb", "near\n"));
    warmLang(root);
    CHECK(lookupName(root, "a/call.rb", "near").path == QLatin1String("a/one.rb"));
}

LangDef bodyAt(const char *text, int line)
{
    return rubyAt(rubyLines(QLatin1String(text)), line, QStringLiteral("ops.rb"));
}

void testRubySetter()
{
    const char *text = "def label=(value)\n  value\nend\n"
                       "def mode(kind = \"end\")\n  kind\nend\n";
    CHECK(bodyAt(text, 1).body.contains(QLatin1String("value")));
    CHECK(bodyAt(text, 4).body.contains(QLatin1String("kind")));
}

void testRubyEndless()
{
    CHECK(bodyAt("def shut(kind = \"end\"); end\n", 1).body.isEmpty());
    CHECK(bodyAt("def plain = 1\n", 1).body.isEmpty());
    CHECK(bodyAt("def plain() = 1\n", 1).body.isEmpty());
}

void testRubyOrder()
{
    const char *text = "def ==(other)\n  1\nend\ndef <=>(other)\n  0\nend\n";
    CHECK(bodyAt(text, 1).body.contains(QLatin1String("1")));
    CHECK(bodyAt(text, 4).body.contains(QLatin1String("0")));
}

void testRubyAmbiguous()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "a/one.rb", "def shared\n  1\nend\n") && putFile(root, "b/two.rb", "def shared\n  2\nend\n")
        && putFile(root, "c.rb", "shared\n"));
    warmLang(root);
    CHECK(!lookupName(root, "c.rb", "shared").found());
}

} // namespace

int rubyPickTests()
{
    testRubyNear();
    testRubyAmbiguous();
    testRubySetter();
    testRubyEndless();
    testRubyOrder();
    return g_fails;
}
