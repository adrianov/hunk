// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "RubyDetail.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

void groupCheck(bool ok, const char *text, int line);

#define CHECK(cond) groupCheck(static_cast<bool>(cond), #cond, __LINE__)

namespace {

bool putFile(const QString &root, const char *rel, const char *text)
{
    const QString path = QDir(root).filePath(QLatin1String(rel));
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(text) >= 0;
}

LangDef at(const QString &root, int line, const char *name)
{
    return lookupDef(defSpot(root, QStringLiteral("app/api/spot.rb"), line, 0, QLatin1String(name), {}));
}

void testParse()
{
    QString name;
    CHECK(rubyHelpersMix(QStringLiteral("      helpers ::APIv1::NamedParams"), &name));
    CHECK(name == QLatin1String("APIv1::NamedParams"));
    CHECK(!rubyHelpersMix(QStringLiteral("      helpers do"), &name));
}

void testJump(const QString &root)
{
    const LangDef mod = at(root, 2, "APIv1::BotOrderAccess");
    CHECK(mod.path == QLatin1String("app/api/bot_order_access.rb") && mod.line == 2);
    const LangDef word = at(root, 2, "helpers");
    CHECK(word.path == mod.path && word.line == 2);
    CHECK(!at(root, 3, "helpers").found());
}

} // namespace

void rubyHelpersTests()
{
    testParse();
    QTemporaryDir dir;
    const QString root = dir.path();
    CHECK(dir.isValid());
    CHECK(putFile(root, "app/api/bot_order_access.rb",
                  "module APIv1\n  module BotOrderAccess\n    def authorize_spot_read!\n    end\n  end\nend\n"));
    CHECK(putFile(root, "app/api/spot.rb", "class Spot\n  helpers ::APIv1::BotOrderAccess\n  helpers do\n  end\nend\n"));
    warmLang(root);
    testJump(root);
}
