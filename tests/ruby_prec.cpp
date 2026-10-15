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

const char *kPrec = "    d.round(settings.precision_for(type), 1)";

LangSpot precSpot(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app/models/market.rb");
    spot.line = 3;
    spot.column = QLatin1String(kPrec).indexOf(QLatin1String("precision_for"));
    spot.name = QStringLiteral("precision_for");
    spot.receiver = QStringLiteral("settings");
    return spot;
}

bool precOpen(const QString &root)
{
    LangSpot spot = precSpot(root);
    spot.receiver = rubyTyped(spot, QLatin1String(kPrec));
    const LangDef def = lookupDef(spot);
    return spot.receiver == QLatin1String("MarketSettings::Base") && def.path.endsWith(QLatin1String("base.rb"))
        && def.signature.contains(QLatin1String("def precision_for"));
}

void precMarket(const QString &root)
{
    CHECK(putFile(root, "app/models/market.rb",
                  "class Market\n  def fix(type, d)\n    d.round(settings.precision_for(type), 1)\n  end\n"
                  "  def settings\n    @settings ||= MarketSettings.for(self)\n  end\nend\n"));
    CHECK(putFile(root, "app/services/market_settings.rb",
                  "module MarketSettings\n  class UnsupportedAPI < StandardError; end\n  class << self\n    def for(market)\n"
                  "      return Local.new(market)\n      Broken.new(market)\n      adapter.new(market)\n"
                  "    end\n    def precision(settings, type)\n      settings.ask_precision\n    end\n  end\nend\n"));
}

void precKinds(const QString &root)
{
    CHECK(putFile(root, "app/services/market_settings/base.rb",
                  "module MarketSettings\n  class Base\n    def precision_for(type)\n      type\n    end\n  end\nend\n"));
    CHECK(putFile(root, "app/services/market_settings/local.rb",
                  "module MarketSettings\n  class Local < Base\n  end\nend\n"));
    CHECK(putFile(root, "app/services/market_settings/broken.rb",
                  "module MarketSettings\n  class Broken < Base\n  end\nend\n"));
}

void precDecoy(const QString &root)
{
    CHECK(putFile(root, "app/other.rb", "class Other\n  def precision_for(type)\n    type\n  end\nend\n"));
    CHECK(putFile(root, "db/schema.rb", "create_table \"markets\" do |t|\n  t.integer \"precision\"\nend\n"));
    CHECK(putFile(root, "app/decoy.rb", "module Other\n  class Local < Missing\n  end\n  class Missing\n"
                                        "    def precision_for(type)\n    end\n  end\nend\n"));
}

bool classOpen(const QString &root)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QStringLiteral("app/models/market.rb");
    spot.line = 3;
    spot.name = QStringLiteral("precision");
    spot.receiver = QStringLiteral("MarketSettings");
    const LangDef def = lookupDef(spot);
    return def.path.endsWith(QLatin1String("market_settings.rb")) && def.signature.contains(QLatin1String("def precision"));
}

void testPrec()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    precMarket(root);
    precKinds(root);
    precDecoy(root);
    warmLang(root);
    CHECK(precOpen(root));
    CHECK(classOpen(root));
}

} // namespace

int rubyPrecTests()
{
    testPrec();
    return g_fails;
}
