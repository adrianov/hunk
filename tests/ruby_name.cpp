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

void testRubyType()
{
    QString name;
    CHECK(rubyTypeName(QStringLiteral("class Telemetry::Metrics::SyncStocks < Telemetry::Metrics::Base"), &name));
    CHECK(name == QLatin1String("Telemetry::Metrics::SyncStocks"));
    CHECK(!rubyTypeName(QStringLiteral("class << self"), &name));
    CHECK(rubyConst(QStringLiteral("Telemetry::Metrics::SyncStocks.duration"), 20, 30) == name);
}

void testAssign()
{
    QString name;
    CHECK(rubyAssign(QStringLiteral("DEFAULT_PAYMENT = 'card'"), &name));
    CHECK(name == QLatin1String("DEFAULT_PAYMENT") && !rubyAssign(QStringLiteral("DEFAULT_PAYMENT == 'card'"), &name));
    CHECK(rubyConst(QStringLiteral("private_constant :DEFAULT_PAYMENT"), 17, 33) == QLatin1String("DEFAULT_PAYMENT"));
}

bool arrayShown(const LangDef &def)
{
    return def.signature.contains(QLatin1String("ho]st")) && def.signature.contains(QLatin1String(":host"))
        && def.signature.trimmed().endsWith(QLatin1String("]")) && !def.signature.contains(QLatin1String("def labels"))
        && def.body.isEmpty();
}

void testConstArray()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "tags.rb",
                  "ACTIVE_REQUEST_ATTRIBUTES = [\n  :one,\n  :two,\n  :three,\n  :four,\n  :five,\n  :six,\n"
                  "  \"ho]st\",\n  :host,\n]\n\ndef labels\nend\n"));
    warmLang(root);
    CHECK(arrayShown(lookupName(root, "use.rb", "ACTIVE_REQUEST_ATTRIBUTES")));
}

void testConstDef()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "pay.rb",
                  "module Shop\n  class Pay\n    # Card\n    DEFAULT_PAYMENT = 'card'\n"
                  "    private_constant :DEFAULT_PAYMENT\n  end\nend\n"));
    warmLang(root);
    const LangDef def = lookupName(root, "use.rb", "Pay::DEFAULT_PAYMENT");
    CHECK(def.path == QLatin1String("pay.rb") && def.line == 4 && def.signature.contains(QLatin1String("'card'"))
          && def.comment.contains(QLatin1String("Card")));
}

bool userModel(const QString &root)
{
    const LangDef model = lookupName(root, "app/services/user/save_promocode_usage_history.rb", "User");
    return model.path == QLatin1String("app/models/user.rb") && model.line == 2 && model.comment.contains(QLatin1String("Account"));
}

void testModel()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "app/models/user.rb", "# Account\nclass User < ApplicationRecord\n  MIN_AGE = 18\nend\n"));
    CHECK(putFile(root, "app/services/user/save_promocode_usage_history.rb",
                  "class User\n  class SavePromocodeUsageHistory\n    def self.call\n    end\n  end\nend\n"));
    warmLang(root);
    CHECK(userModel(root));
}

bool nestedClass(const QString &root)
{
    const LangDef def = lookupName(root, "karafka.rb", "Telemetry::KarafkaMonitor");
    return def.path == QLatin1String("lib/telemetry/karafka_monitor.rb") && def.line == 3 && def.comment.isEmpty()
        && def.signature.startsWith(QLatin1String("module Telemetry\n  # monitor\n"))
        && def.signature.contains(QLatin1String("class KarafkaMonitor")) && def.body.contains(QLatin1String("1"))
        && def.body.endsWith(QLatin1String("\nend"));
}

void testNestedClass()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "karafka.rb", "config.monitor = Telemetry::KarafkaMonitor.new\n"));
    CHECK(putFile(root, "lib/telemetry/karafka_monitor.rb",
                  "module Telemetry\n  # monitor\n  class KarafkaMonitor < Karafka::Instrumentation::Monitor\n    def instrument\n      1\n    end\n  end\nend\n"));
    warmLang(root);
    CHECK(nestedClass(root));
}

LangDef lookupAt(const QString &root, const char *path, int line, int column, const char *name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.line = line;
    spot.column = column;
    spot.name = QLatin1String(name);
    return lookupDef(spot);
}

bool localQuiet(const QString &root)
{
    const LangDef local = lookupAt(root, "path_normalizer.rb", 8, 4, "normalized");
    const LangDef block = lookupAt(root, "recorder.rb", 6, 6, "normalized");
    const LangDef after = lookupAt(root, "recorder.rb", 8, 4, "normalized");
    const LangDef method = lookupAt(root, "other.rb", 1, 0, "normalized");
    return !local.found() && !block.found() && after.path == QLatin1String("path_normalizer.rb") && after.line == 2
        && method.path == after.path && method.line == after.line;
}

void testLocal()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "path_normalizer.rb",
                  "class PathNormalizer\n  def normalized(prices)\n    prices\n  end\n  def call\n"
                  "    normalized = path.dup\n\n    normalized.gsub!(\"a\", \"b\")\n  end\nend\n"));
    CHECK(putFile(root, "recorder.rb",
                  "class Recorder\n  def normalize(attributes)\n"
                  "    attributes.each_with_object({}) do |(key, value), normalized|\n      next if value.nil?\n\n"
                  "      normalized[key]\n    end\n    normalized\n  end\nend\n"));
    CHECK(putFile(root, "other.rb", "normalized(1)\n"));
    warmLang(root);
    CHECK(localQuiet(root));
}

void testRubyClass()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "lib/sync_stocks.rb",
                  "# frozen_string_literal: true\n\n# Stages\n"
                  "class Telemetry::Metrics::SyncStocks < Base\n  histogram(:duration)\nend\n"));
    warmLang(root);
    const LangDef def = lookupName(root, "call.rb", "Telemetry::Metrics::SyncStocks");
    CHECK(def.line == 4 && def.comment.contains(QLatin1String("Stages"))
          && def.body.contains(QLatin1String("histogram(:duration)")));
}

} // namespace

int rubyRecvTests();

int rubyNameTests()
{
    testRubyType();
    testAssign();
    testConstDef();
    testConstArray();
    testModel();
    testNestedClass();
    testLocal();
    testRubyClass();
    return g_fails + rubyRecvTests();
}
