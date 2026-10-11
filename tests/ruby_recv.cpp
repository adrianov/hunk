// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "LangTune.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThreadPool>

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

bool goldColumn(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app/models/user.rb");
    spot.line = 5;
    spot.name = QStringLiteral("product_gold_erp_id");
    const LangDef def = lookupDef(spot);
    return def.comment.contains(QLatin1String("favorites")) && def.signature.contains(QLatin1String("null: false"));
}

bool sessionColumn(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app/models/user.rb");
    spot.line = 2;
    spot.name = QStringLiteral("manzana_session_id");
    return lookupDef(spot).comment.contains(QLatin1String("users"));
}

void goldFiles(const QString &root)
{
    CHECK(putFile(root, "db/schema.rb",
                  "create_table \"cart_items\" do |t|\n  t.string \"product_gold_erp_id\", null: false\nend\n"
                  "create_table \"favorites\" do |t|\n  t.string \"product_gold_erp_id\", null: false\nend\n"
                  "create_table \"users\" do |t|\n  t.string \"manzana_session_id\"\nend\n"
                  "create_table \"sessions\" do |t|\n  t.string \"manzana_session_id\"\nend\n"));
    CHECK(putFile(root, "app/models/user.rb",
                  "def manzana?\n  manzana_session_id.present?\nend\ndef ids\n  favorites.pluck(:product_gold_erp_id)\nend\n"));
}

void testBareColumn()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    goldFiles(root);
    warmLang(root);
    CHECK(goldColumn(root) && sessionColumn(root));
    LangSpot quiet;
    quiet.root = root;
    quiet.path = QStringLiteral("app/services/note.rb");
    quiet.name = QStringLiteral("product_gold_erp_id");
    CHECK(!lookupDef(quiet).found());
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

LangDef superAt(const QString &root, const char *path, int line)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.line = line;
    spot.name = QStringLiteral("super");
    return lookupDef(spot);
}

bool parentSuper(const QString &root)
{
    const LangDef def = superAt(root, "child.rb", 8);
    return def.path == QLatin1String("child.rb") && def.line == 2 && def.body.contains(QLatin1String("1"));
}

bool prependSuper(const QString &root)
{
    const LangDef def = superAt(root, "plug.rb", 3);
    return def.path == QLatin1String("only.rb") && def.body.contains(QLatin1String("2"));
}

bool manySuper(const QString &root)
{
    const LangDef def = superAt(root, "plug.rb", 3);
    return def.signature.contains(QLatin1String("a.rb:")) && def.signature.contains(QLatin1String("b.rb:"))
        && def.signature.contains(QLatin1String("3")) && def.signature.contains(QLatin1String("4"));
}

void superFiles(const QString &root)
{
    CHECK(putFile(root, "child.rb", "class Parent\n  def call\n    1\n  end\nend\nclass Child < Parent\n  def call\n    super\n  end\nend\n"));
    CHECK(putFile(root, "plug.rb", "module Plug\n  def call\n    super\n  end\nend\n"));
    CHECK(putFile(root, "only.rb", "class Only\n  prepend Plug\n  def call\n    2\n  end\nend\n"));
}

void manyFiles(const QString &root)
{
    CHECK(putFile(root, "plug.rb", "module Plug\n  def call\n    super\n  end\nend\n"));
    CHECK(putFile(root, "a.rb", "class A\n  prepend Plug\n  def call\n    3\n  end\nend\n"));
    CHECK(putFile(root, "b.rb", "class B\n  prepend Plug\n  def call\n    4\n  end\nend\n"));
}

void testSuper()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    superFiles(root);
    warmLang(root);
    CHECK(parentSuper(root) && prependSuper(root));
}

bool batchHit(const QString &root, const char *path, const char *name, const char *bit)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.name = QLatin1String(name);
    return langSought(root, spot.name) && lookupDef(spot).body.contains(QLatin1String(bit));
}

void seekPair(const QString &root)
{
    QThreadPool pool;
    pool.start([&root] { langEnsure(root, QStringLiteral("foo")); });
    pool.start([&root] { langEnsure(root, QStringLiteral("bar")); });
    pool.waitForDone();
}

void testSeekBatch()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "a.rb", "def foo\n  1\nend\n"));
    CHECK(putFile(root, "b.rb", "def bar\n  2\nend\n"));
    warmLang(root);
    seekPair(root);
    CHECK(batchHit(root, "a.rb", "foo", "1") && batchHit(root, "b.rb", "bar", "2"));
}

void testManySuper()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    manyFiles(root);
    warmLang(root);
    CHECK(manySuper(root));
}

int rubyRecvTests()
{
    testSchemaColumn();
    testSuper();
    testManySuper();
    testSeekBatch();
    testBareColumn();
    testClassCall();
    testDsl();
    testMetric();
    testLooseMetric();
    return g_fails;
}
