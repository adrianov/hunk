// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "RubyDetail.hpp"

#include <QTemporaryDir>

void groupCheck(bool ok, const char *text, int line);
LangDef groupLookup(const QString &root, const char *path, int line, int column, const char *name);
bool groupWarm(const QString &root);
int groupColumn(const char *line, const char *part);
bool groupHit(const QString &root, int line, int column, const char *name, int defLine, const char *part);

#define CHECK(cond) groupCheck(static_cast<bool>(cond), #cond, __LINE__)

namespace {

const char *kLike = "  it_behaves_like('rejects a request without service authentication', :post, '/api/v1/spot/cancel_order')";
const char *kSplit = "    'rejects a request without service authentication',";
const char *kReject = "rejects a request without service authentication";
const char *kPhrase = "when the instance processes bot orders";

void checkLike(const QString &root)
{
    CHECK(groupHit(root, 2, groupColumn(kLike, kReject), kReject, 8, "expect(verb)"));
    const int pathAt = groupColumn(kLike, "/api");
    CHECK(rubyGroupName(root, QStringLiteral("spec/api/cancel_spec.rb"), 2, pathAt).isEmpty());
    CHECK(!groupLookup(root, "spec/api/cancel_spec.rb", 2, pathAt, "/api/v1/spot/cancel_order").found());
}

void checkSplit(const QString &root)
{
    CHECK(groupHit(root, 5, groupColumn("  it_behaves_like(", "it_behaves_like"), "it_behaves_like", 8, "expect(verb)"));
    CHECK(rubyGroupName(root, QStringLiteral("spec/api/cancel_spec.rb"), 6, groupColumn(kSplit, kReject)) == QLatin1String(kReject));
    CHECK(groupHit(root, 6, groupColumn(kSplit, kReject), kReject, 8, "expect(verb)"));
    CHECK(rubyGroupName(root, QStringLiteral("spec/api/cancel_spec.rb"), 8, groupColumn("    '/api/v1/spot/cancel_order'", "/api")).isEmpty());
}

void checkLogs(const QString &root)
{
    CHECK(groupHit(root, 3, groupColumn("  include_examples 'logs the call'", "logs the call"), "logs the call", 12, "1"));
    CHECK(groupHit(root, 4, groupColumn("  it_should_behave_like :logs", ":logs") + 1, "logs", 16, "2"));
    const LangDef method = groupLookup(root, "other/use.rb", 0, 0, "logs");
    CHECK(method.path == QLatin1String("app/logs.rb") && method.body.contains(QLatin1String("3")));
}

} // namespace

void rubyLikeTests()
{
    QTemporaryDir dir;
    const QString root = dir.path();
    CHECK(dir.isValid() && groupWarm(root));
    checkLike(root);
    checkSplit(root);
    checkLogs(root);
    CHECK(!groupLookup(root, "spec/support/bot.rb", 4, groupColumn("RSpec.shared_context 'when the instance processes bot orders' do", kPhrase), kPhrase).found());
}
