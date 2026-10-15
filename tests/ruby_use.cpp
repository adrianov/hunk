// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"

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
    return file.write(text) >= 0;
}

LangSpot spotAt(const QString &root, const char *path, int line, const char *name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.line = line;
    spot.name = QLatin1String(name);
    return spot;
}

void testClassUse(const QString &root)
{
    const LangSpot spot = spotAt(root, "lib/telemetry/metrics/counter.rb", 1, "Telemetry::Metrics::Counter");
    CHECK(!lookupDef(spot).found());
    const LangDef use = lookupUse(spot);
    CHECK(use.path == QLatin1String("app/record.rb"));
    CHECK(use.line == 1);
}

void testBareUse(const QString &root)
{
    const LangSpot spot = spotAt(root, "app/models/concerns/order_history.rb", 5, "status_change_context");
    CHECK(!lookupDef(spot).found());
    const LangDef use = lookupUse(spot);
    CHECK(use.line == 3);
    CHECK(use.signature.contains(QLatin1String("context: status_change_context")));
}
void testShortUse(const QString &root)
{
    const LangSpot spot = spotAt(root, "app/services/products/catalog_source.rb", 2, "model");
    CHECK(!lookupDef(spot).found());
    const LangDef use = lookupUse(spot);
    CHECK(use.path == QLatin1String("app/services/products/find.rb"));
    CHECK(use.signature.contains(QLatin1String("CatalogSource.model")));
}
void testMethodUse(const QString &root)
{
    const LangSpot spot = spotAt(root, "lib/telemetry/metrics/counter.rb", 4, "add");
    CHECK(!lookupDef(spot).found());
    const LangDef use = lookupUse(spot);
    CHECK(use.path == QLatin1String("app/record.rb"));
    CHECK(use.line == 2);
    CHECK(use.signature.contains(QLatin1String(".add")));
}

void testInitMore(const QString &root)
{
    const LangSpot spot = spotAt(root, "lib/telemetry/metrics/counter.rb", 2, "initialize");
    CHECK(!lookupDef(spot).found());
    CHECK(moreUse(spot) == QLatin1String("Telemetry::Metrics::Counter"));
}

void testInitUse(const QString &root)
{
    testInitMore(root);
    const LangSpot spot = spotAt(root, "lib/telemetry/metrics/counter.rb", 2, "initialize");
    CHECK(lookupDef(spotAt(root, "app/record.rb", 1, "Telemetry::Metrics::Counter")).line == 1);
    CHECK(lookupUse(spot).path == QLatin1String("app/record.rb"));
    CHECK(lookupUse(spot).signature.contains(QLatin1String(".new")));
}

void putHistory(const QString &root)
{
    const char *history = "module OrderHistory\n"
                          "  def log\n"
                          "    details = { context: status_change_context }\n"
                          "  end\n"
                          "  def status_change_context\n"
                          "    Current.status_change_context\n"
                          "  end\n"
                          "end\n";
    CHECK(putFile(root, "app/models/concerns/order_history.rb", history));
}
void putModel(const QString &root)
{
    const char *source = "class Products::CatalogSource\n"
                         "  def self.model = postgres?\n"
                         "end\n";
    const char *find = "include Dry::Monads[:result]\n"
                       "fallback = CatalogSource.model\n";
    const char *sourceSpec = "RSpec.describe Products::CatalogSource do\n"
                             "  expect(described_class.model).to eq(1)\n"
                             "end\n";
    CHECK(putFile(root, "app/services/products/catalog_source.rb", source));
    CHECK(putFile(root, "app/services/products/find.rb", find));
    CHECK(putFile(root, "spec/services/products/catalog_source_spec.rb", sourceSpec));
}

void putCounter(const QString &root)
{
    const char *counter = "class Telemetry::Metrics::Counter\n"
                          "  def initialize(instrument, name:)\n"
                          "  end\n"
                          "  def add(value:)\n"
                          "  end\n"
                          "end\n";
    const char *record = "metric = Telemetry::Metrics::Counter.new(instrument, name: \"rows\")\n"
                         "metric.add(value: 1)\n";
    const char *spec = "Telemetry::Metrics::Counter.new(nil, name: \"x\")\n";
    const char *other = "basket.add(value: 1)\n";
    CHECK(putFile(root, "lib/telemetry/metrics/counter.rb", counter));
    CHECK(putFile(root, "app/record.rb", record));
    CHECK(putFile(root, "spec/counter_spec.rb", spec));
    CHECK(putFile(root, "app/other.rb", other));
}

} // namespace

int rubyTypeTests();

int rubyUseTests()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    putCounter(root);
    putModel(root);
    putHistory(root);
    warmLang(root);
    testInitUse(root);
    testClassUse(root);
    testMethodUse(root);
    testShortUse(root);
    testBareUse(root);
    return g_fails + rubyTypeTests();
}
