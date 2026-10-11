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

void testConstDef()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(putFile(root, "pay.rb", "# Card\nDEFAULT_PAYMENT = 'card'\nprivate_constant :DEFAULT_PAYMENT\n"));
    warmLang(root);
    const LangDef def = lookupName(root, "use.rb", "DEFAULT_PAYMENT");
    CHECK(def.path == QLatin1String("pay.rb") && def.line == 2 && def.signature.contains(QLatin1String("'card'"))
          && def.comment.contains(QLatin1String("Card")));
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
    testRubyClass();
    return g_fails + rubyRecvTests();
}
