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
    return file.write(QLatin1String(text).toUtf8()) >= 0;
}

void schemaFiles(const QString &root)
{
    CHECK(putFile(root, "db/schema.rb",
                  "create_table \"online_orders\" do |t|\n  t.float \"final_delivery_price\", precision: 8, scale: 2\nend\n"
                  "create_table \"addresses\" do |t|\n  t.string \"city\"\nend\n"
                  "create_table \"statuses\" do |t|\n  t.string \"name\"\nend\n"));
    CHECK(putFile(root, "db/clickhouse_schema.rb", "create_table \"prices\" do |t|\n  t.float \"current\"\nend\n"));
}

LangDef columnDef(const QString &root, const QString &name, const QString &receiver)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app/build.rb");
    spot.name = name;
    spot.receiver = receiver;
    return lookupDef(spot);
}

bool orderColumn(const QString &root)
{
    const LangDef def = columnDef(root, QStringLiteral("final_delivery_price"), QStringLiteral("@online_order"));
    return def.path == QLatin1String("db/schema.rb") && def.signature.contains(QLatin1String("precision: 8"))
        && def.comment.contains(QLatin1String("online_orders"));
}

bool priceColumn(const QString &root)
{
    return columnDef(root, QStringLiteral("current"), QStringLiteral("price")).path.endsWith(QLatin1String("clickhouse_schema.rb"))
        && columnDef(root, QStringLiteral("current"), QStringLiteral("prices")).path.endsWith(QLatin1String("clickhouse_schema.rb"));
}

bool sColumn(const QString &root)
{
    return columnDef(root, QStringLiteral("city"), QStringLiteral("address")).comment.contains(QLatin1String("addresses"))
        && columnDef(root, QStringLiteral("name"), QStringLiteral("status")).comment.contains(QLatin1String("statuses"));
}

void testSchemaColumn()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    schemaFiles(root);
    warmLang(root);
    CHECK(orderColumn(root) && priceColumn(root) && sColumn(root));
}

void callFiles(const QString &root)
{
    CHECK(putFile(root, "app/services/carts/build.rb", "class Build\n  def self.call\n    1\n  end\nend\n"));
    CHECK(putFile(root, "app/services/products/fields.rb",
                  "module Products\n  class AdditionalFieldsFromPostgres\n    def self.call\n      2\n    end\n  end\nend\n"));
}

bool classCall(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app/services/carts/build.rb");
    spot.name = QStringLiteral("call");
    spot.receiver = QStringLiteral("Products::AdditionalFieldsFromPostgres");
    const LangDef def = lookupDef(spot);
    return def.path.endsWith(QLatin1String("fields.rb")) && def.body.contains(QLatin1String("2"));
}

void testClassCall()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    callFiles(root);
    warmLang(root);
    CHECK(classCall(root));
}

bool metricHit(const QString &root, const char *name, const char *bit)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String("app/build.rb");
    spot.name = QLatin1String(name);
    spot.receiver = QLatin1String("Telemetry::Metrics::VirtualProducts");
    const LangDef def = lookupDef(spot);
    return def.signature.contains(QLatin1String(bit)) && !def.signature.contains(QLatin1String("class "));
}

bool dslHit(const QString &root, const char *name, const char *bit)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String("app/post.rb");
    spot.name = QLatin1String(name);
    spot.receiver = QLatin1String("Post");
    return lookupDef(spot).signature.contains(QLatin1String(bit));
}

void testDsl()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "app/models/post.rb",
                  "class Post\n  has_many :comments\n  has_one :author\n  def title\n    has_many :nope\n  end\n"
                  "  enum status: { open: 0 }\nend\n"));
    warmLang(root);
    CHECK(dslHit(root, "comments", "has_many") && dslHit(root, "author", "has_one") && dslHit(root, "status", "enum"));
    CHECK(!dslHit(root, "nope", "has_many"));
}

bool looseMetric(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("spec/http_spec.rb");
    spot.name = QStringLiteral("active_requests");
    spot.receiver = QStringLiteral("http");
    const LangDef def = lookupDef(spot);
    return def.path.endsWith(QLatin1String("metrics/http.rb")) && def.signature.contains(QLatin1String("up_down(:active_requests"));
}

void testLooseMetric()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "lib/telemetry/metrics/http.rb",
                  "class Telemetry::Metrics::Http < Base\n  up_down(:active_requests, 'client')\nend\n"));
    CHECK(putFile(root, "lib/telemetry/metrics/server.rb",
                  "class Telemetry::Metrics::Server < Base\n  up_down(:active_requests, 'server')\nend\n"));
    warmLang(root);
    CHECK(looseMetric(root));
}

void testMetric()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "lib/telemetry/metrics/virtual_products.rb",
                  "class Telemetry::Metrics::VirtualProducts < Base\n"
                  "  counter(:missing_items, 'Missing items', unit: '{item}')\n"
                  "  histogram(:operation_duration, 'Duration',\n            buckets: [1])\nend\n"));
    warmLang(root);
    CHECK(metricHit(root, "missing_items", "counter(:missing_items") && metricHit(root, "operation_duration", "buckets:"));
}

} // namespace

int rubyRecvTests()
{
    testSchemaColumn();
    testClassCall();
    testDsl();
    testMetric();
    testLooseMetric();
    return g_fails;
}
