// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "RubyDetail.hpp"

#include <QTemporaryDir>

void groupCheck(bool ok, const char *text, int line);
LangDef groupLookup(const QString &root, const char *path, int line, int column, const char *name);
bool groupPut(const QString &root, const char *rel, const QString &text);
int groupColumn(const char *line, const char *part);

#define CHECK(cond) groupCheck(static_cast<bool>(cond), #cond, __LINE__)

namespace {

const char *kSpec = "RSpec.describe 'trades' do\n"
                    "  let!(:older_trade) do\n"
                    "    create(:trade, tid: 1)\n"
                    "  end\n"
                    "\n"
                    "  let(:request_params) { { start_id: older_trade.tid } }\n"
                    "\n"
                    "  it 'filters' do\n"
                    "    expect(older_trade.tid).to eq(1)\n"
                    "    bot_request(:get, '/trades', request_params)\n"
                    "  end\n"
                    "end\n";

const char *kSupport = "RSpec.shared_context 'shared trades' do\n"
                       "  let(:other_trade) { create(:trade, tid: 2) }\n"
                       "end\n"
                       "\n"
                       "shared_examples :trade do\n"
                       "  it { 1 }\n"
                       "end\n"
                       "\n"
                       "let(:trade) { 2 }\n";

const char *kUse = "  include_context 'shared trades'\n"
                   "  include_examples :trade\n"
                   "  it { expect(other_trade.tid).to eq(2) }\n";

bool letName(const char *text, const char *want)
{
    QString name;
    return rubyLet(QLatin1String(text), &name) && name == QLatin1String(want);
}

LangDef look(const QString &root, const char *path, int line, const char *row, const char *name)
{
    return groupLookup(root, path, line, groupColumn(row, name), name);
}

void checkParse()
{
    CHECK(letName("let!(:older_trade) do", "older_trade"));
    CHECK(letName("  let(:request_params) { { a: 1 } }", "request_params"));
    CHECK(letName("let 'quoted' do", "quoted"));
    CHECK(!letName("letter(:x) do", "x"));
    CHECK(rubyLetStart(QStringLiteral("let!(:older_trade) do")));
    CHECK(rubyNamedStart(QStringLiteral("let(:request_params) { 1 }")));
}

void checkBang(const QString &root)
{
    const LangDef def = look(root, "spec/trades_spec.rb", 9, "    expect(older_trade.tid).to eq(1)", "older_trade");
    CHECK(def.path == QLatin1String("spec/trades_spec.rb") && def.line == 2);
    CHECK(def.signature.contains(QLatin1String("let!(:older_trade)")));
    CHECK(def.body.contains(QLatin1String("create(:trade,")));
    CHECK(!look(root, "spec/trades_spec.rb", 2, "  let!(:older_trade) do", "older_trade").found());
}

void checkParams(const QString &root)
{
    const LangDef def =
        look(root, "spec/trades_spec.rb", 10, "    bot_request(:get, '/trades', request_params)", "request_params");
    CHECK(def.path == QLatin1String("spec/trades_spec.rb") && def.line == 6);
    CHECK(def.signature.contains(QLatin1String("let(:request_params)")));
    CHECK(def.body.isEmpty());
}

void checkShared(const QString &root)
{
    const LangDef def =
        look(root, "spec/api/use_spec.rb", 3, "  it { expect(other_trade.tid).to eq(2) }", "other_trade");
    CHECK(def.path == QLatin1String("spec/support/trades.rb") && def.line == 2);
    CHECK(def.signature.contains(QLatin1String("let(:other_trade)")));
}

void checkNotLet(const QString &root)
{
    const char *row = "  include_examples :trade";
    const LangDef def = look(root, "spec/api/use_spec.rb", 2, row, "trade");
    CHECK(def.signature.contains(QLatin1String("shared_examples :trade")));
    CHECK(!def.signature.contains(QLatin1String("let(:trade)")));
}

bool warmLets(const QString &root)
{
    return groupPut(root, "spec/trades_spec.rb", QString::fromLatin1(kSpec))
        && groupPut(root, "spec/support/trades.rb", QString::fromLatin1(kSupport))
        && groupPut(root, "spec/api/use_spec.rb", QString::fromLatin1(kUse));
}

} // namespace

void rubyLetTests()
{
    checkParse();
    QTemporaryDir dir;
    CHECK(dir.isValid() && warmLets(dir.path()));
    warmLang(dir.path());
    checkBang(dir.path());
    checkParams(dir.path());
    checkShared(dir.path());
    checkNotLet(dir.path());
}
