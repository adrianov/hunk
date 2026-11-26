// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

void groupCheck(bool ok, const char *text, int line);
LangDef groupLookup(const QString &root, const char *path, int line, int column, const char *name);
int groupColumn(const char *line, const char *part);

#define CHECK(cond) groupCheck(static_cast<bool>(cond), #cond, __LINE__)

namespace {

const char *kNamed = "module APIv1\n"
                     "  module NamedParams\n"
                     "    params :pagination_filters do\n"
                     "      optional :limit, type: Integer\n"
                     "    end\n"
                     "\n"
                     "    params :bot_task do\n"
                     "      optional :bot_task_id, type: String\n"
                     "    end\n"
                     "\n"
                     "    params :order do\n"
                     "      requires :side, type: String\n"
                     "    end\n"
                     "  end\n"
                     "end\n";

const char *kSpot = "params do\n"
                    "  use :pagination_filters, :bot_task\n"
                    "  use :order\n"
                    "  use BotServiceMiddleware\n"
                    "end\n"
                    "\n"
                    "def order\n"
                    "  1\n"
                    "end\n"
                    "\n"
                    "order\n";

const char *kUse = "  use :pagination_filters, :bot_task";

bool putFile(const QString &root, const char *rel, const char *text)
{
    const QString path = QDir(root).filePath(QLatin1String(rel));
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(text) >= 0;
}

bool paramsHit(const QString &root, int line, int column, const char *name, int defLine, const char *part)
{
    const LangDef def = groupLookup(root, "app/api/api_v1/endpoints/spot.rb", line, column, name);
    return def.path == QLatin1String("app/api/api_v1/named_params.rb") && def.line == defLine
        && def.body.contains(QLatin1String(part));
}

void testParse()
{
    QString name;
    CHECK(rubyParams(QStringLiteral("params :bot_task do"), &name) && name == QLatin1String("bot_task"));
    CHECK(rubyParamsStart(QStringLiteral("params :pagination_filters do")));
    CHECK(!rubyParamsStart(QStringLiteral("params do")));
    CHECK(!rubyParams(QStringLiteral("use :bot_task"), &name));
}

void testUseHits(const QString &root)
{
    CHECK(paramsHit(root, 2, groupColumn(kUse, "pagination_filters"), "pagination_filters", 3, "limit"));
    CHECK(paramsHit(root, 2, groupColumn(kUse, "bot_task"), "bot_task", 7, "bot_task_id"));
    CHECK(paramsHit(root, 2, groupColumn(kUse, "use"), "use", 3, "limit"));
    CHECK(paramsHit(root, 3, groupColumn("  use :order", "order"), "order", 11, "side"));
}

void testRackAndOrder(const QString &root)
{
    CHECK(!groupLookup(root, "app/api/api_v1/endpoints/spot.rb", 4,
                       groupColumn("  use BotServiceMiddleware", "use"), "use")
               .found());
    const LangDef call = groupLookup(root, "app/api/api_v1/endpoints/spot.rb", 11, groupColumn("order", "order"),
                                     "order");
    CHECK(call.path == QLatin1String("app/api/api_v1/endpoints/spot.rb") && call.line == 7
          && call.body.contains(QLatin1String("1")));
}

} // namespace

void rubyGrapeTests()
{
    testParse();
    QTemporaryDir dir;
    const QString root = dir.path();
    CHECK(dir.isValid());
    CHECK(putFile(root, "app/api/api_v1/named_params.rb", kNamed));
    CHECK(putFile(root, "app/api/api_v1/endpoints/spot.rb", kSpot));
    warmLang(root);
    testUseHits(root);
    testRackAndOrder(root);
}
