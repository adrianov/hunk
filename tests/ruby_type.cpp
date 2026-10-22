// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "LangTune.hpp"
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

bool putFile(const QString &root, const char *rel, const char *text)
{
    const QString path = QDir(root).filePath(QLatin1String(rel));
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(QLatin1String(text).toUtf8()) >= 0;
}

const char *kCall = "    parent.store_leg_quantity!";

LangSpot legSpot(const QString &root, const char *path, int line)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.line = line;
    spot.column = QLatin1String(kCall).indexOf(QLatin1String("store_leg_quantity!"));
    spot.name = QStringLiteral("store_leg_quantity!");
    spot.receiver = QStringLiteral("parent");
    return spot;
}

bool legHit(const LangDef &def)
{
    return def.path.endsWith(QLatin1String("exchanger.rb"))
        && def.signature.contains(QLatin1String("def store_leg_quantity!"));
}

bool legOpen(const QString &root, const char *path, int line)
{
    LangSpot spot = legSpot(root, path, line);
    spot.receiver = rubyTyped(spot, QLatin1String(kCall));
    return spot.receiver == QLatin1String("Exchanger") && legHit(lookupDef(spot));
}

bool legShut(const QString &root, const char *path, int line)
{
    LangSpot spot = legSpot(root, path, line);
    return rubyTyped(spot, QLatin1String(kCall)).isEmpty() && !lookupDef(spot).found();
}

void legModels(const QString &root)
{
    CHECK(putFile(root, "app/models/exchanger.rb", "class Exchanger\n  def store_leg_quantity!\n    1\n  end\nend\n"));
    CHECK(putFile(root, "app/models/other.rb", "class Other\n  def store_leg_quantity!\n    2\n  end\nend\n"));
}

void testAssoc()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    legModels(root);
    CHECK(putFile(root, "app/models/order_trigger.rb",
                  "class OrderTrigger\n  belongs_to :tpsl_exchanger, class_name: \"Exchanger\"\nend\n"));
    CHECK(putFile(root, "app/execute.rb",
                  "class Execute\n  def sync(order_trigger)\n    parent = order_trigger.tpsl_exchanger\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    warmLang(root);
    CHECK(legOpen(root, "app/execute.rb", 4));
}

void testSafe()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    legModels(root);
    CHECK(putFile(root, "app/models/order_trigger.rb",
                  "class OrderTrigger\n  belongs_to :tpsl_exchanger, class_name: \"Exchanger\"\nend\n"));
    CHECK(putFile(root, "app/safe.rb",
                  "class Execute\n  def sync(order_trigger)\n    parent = order_trigger&.tpsl_exchanger\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    warmLang(root);
    CHECK(legOpen(root, "app/safe.rb", 4));
}

void testDirect()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    legModels(root);
    CHECK(putFile(root, "app/direct.rb",
                  "class Execute\n  def sync\n    parent = Exchanger.find_by(id: 1)\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    warmLang(root);
    CHECK(legOpen(root, "app/direct.rb", 4));
}

void testOr()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    legModels(root);
    CHECK(putFile(root, "app/or.rb",
                  "class Execute\n  def sync\n    parent ||= Exchanger.find_by(id: 1)\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    CHECK(putFile(root, "app/eq.rb",
                  "class Execute\n  def sync\n    parent == Exchanger.find_by(id: 1)\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    warmLang(root);
    CHECK(legOpen(root, "app/or.rb", 4) && legShut(root, "app/eq.rb", 4));
}

void testMiss()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    legModels(root);
    CHECK(putFile(root, "app/miss.rb",
                  "class Execute\n  def sync(order_trigger)\n    parent = order_trigger.missing\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    warmLang(root);
    CHECK(legShut(root, "app/miss.rb", 4) && legShut(root, "app/miss.rb", 40));
}

void testBody()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    legModels(root);
    CHECK(putFile(root, "app/models/order_trigger.rb",
                  "class OrderTrigger\n  def tpsl_exchanger\n"
                  "    @tpsl_exchanger ||= Exchanger.find_by(id: exchanger_id)\n  end\nend\n"));
    CHECK(putFile(root, "app/execute.rb",
                  "class Execute\n  def sync(order_trigger)\n    parent = order_trigger.tpsl_exchanger\n"
                  "    parent.store_leg_quantity!\n  end\nend\n"));
    warmLang(root);
    CHECK(legOpen(root, "app/execute.rb", 4));
}

} // namespace

int rubyPrecTests();
int rubyScopeTests();

int rubyTypeTests()
{
    testAssoc();
    testSafe();
    testDirect();
    testOr();
    testMiss();
    testBody();
    return g_fails + rubyPrecTests() + rubyScopeTests();
}
