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

bool putFile(const QString &root, const char *rel, const char *text)
{
    const QString path = QDir(root).filePath(QLatin1String(rel));
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(QLatin1String(text).toUtf8()) >= 0;
}

const char *kScope = "    scope.public_find(1)";

// A has_many, a scope with decoys of the same names, and a local assigned from that chain.
const char *kFiles[][2] = {
    {"app/models/member.rb", "class Member\n  has_many :exchanger_triggers\nend\n"},
    {"app/models/exchanger_trigger.rb", "class ExchangerTrigger\n  scope :active, -> { where(state: 1) }\n"
                                        "  class << self\n    def public_find(id)\n      find_by(id:)\n    end\n  end\nend\n"},
    {"app/models/order.rb",
     "class Order\n  scope :active, -> { where(state: 1) }\n  def self.public_find(id)\n    id\n  end\nend\n"},
    {"app/save.rb", "class Save\n  def active\n    @active ||= 1\n  end\nend\n"},
    {"app/endpoint.rb", "class Endpoint\n  def find\n    scope = current_user.exchanger_triggers.active\n"
                        "    scope.public_find(1)\n  end\nend\n"},
};

bool scopeOpen(const QString &root)
{
    LangSpot spot = defSpot(root, QStringLiteral("app/endpoint.rb"), 4,
                            QLatin1String(kScope).indexOf(QLatin1String("public_find")), QStringLiteral("public_find"),
                            QStringLiteral("scope"));
    spot.receiver = rubyTyped(spot, QLatin1String(kScope));
    return spot.receiver == QLatin1String("ExchangerTrigger")
        && lookupDef(spot).path == QLatin1String("app/models/exchanger_trigger.rb");
}

} // namespace

int rubyScopeTests()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    for (const auto &file : kFiles)
        CHECK(putFile(dir.path(), file[0], file[1]));
    warmLang(dir.path());
    CHECK(scopeOpen(dir.path()));
    return g_fails;
}
