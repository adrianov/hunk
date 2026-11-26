// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "LangDef.hpp"
#include "RubyDetail.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

static int g_groupFails = 0;

void groupCheck(bool ok, const char *text, int line)
{
    if (ok)
        return;
    qCritical() << "fail" << text << "at" << line;
    ++g_groupFails;
}

#define CHECK(cond) groupCheck(static_cast<bool>(cond), #cond, __LINE__)

const char *kSupport = "include_context 'when the instance processes bot orders'\n"
                       "\n"
                       "# bots\n"
                       "RSpec.shared_context 'when the instance processes bot orders' do\n"
                       "  let(:task_order) { 1 }\n"
                       "end\n"
                       "\n"
                       "shared_examples 'rejects a request without service authentication' do |verb, path|\n"
                       "  it { expect(verb).to eq(path) }\n"
                       "end\n"
                       "\n"
                       "shared_examples_for 'logs the call' do\n"
                       "  it { 1 }\n"
                       "end\n"
                       "\n"
                       "shared_examples :logs do\n"
                       "  it { 2 }\n"
                       "end\n";

const char *kSpec = "  include_context 'when the instance processes bot orders'\n"
                    "  it_behaves_like('rejects a request without service authentication', :post, '/api/v1/spot/cancel_order')\n"
                    "  include_examples 'logs the call'\n"
                    "  it_should_behave_like :logs\n"
                    "  it_behaves_like(\n"
                    "    'rejects a request without service authentication',\n"
                    "    :post,\n"
                    "    '/api/v1/spot/cancel_order'\n"
                    "  )\n";

LangDef groupLookup(const QString &root, const char *path, int line, int column, const char *name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.line = line;
    spot.column = column;
    spot.name = QLatin1String(name);
    return lookupDef(spot);
}

bool groupPut(const QString &root, const char *rel, const QString &text)
{
    const QString path = QDir(root).filePath(QLatin1String(rel));
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(text.toUtf8()) >= 0;
}

int groupColumn(const char *line, const char *part)
{
    return QString(QLatin1String(line)).indexOf(QLatin1String(part));
}

bool groupHit(const QString &root, int line, int column, const char *name, int defLine, const char *part)
{
    const LangDef def = groupLookup(root, "spec/api/cancel_spec.rb", line, column, name);
    return def.path == QLatin1String("spec/support/bot.rb") && def.line == defLine && def.body.contains(QLatin1String(part));
}

bool groupWarm(const QString &root)
{
    const bool support = groupPut(root, "spec/support/bot.rb", QString::fromLatin1(kSupport));
    const bool spec = groupPut(root, "spec/api/cancel_spec.rb", QString::fromLatin1(kSpec));
    const bool method = groupPut(root, "app/logs.rb", QStringLiteral("def logs\n  3\nend\n"));
    if (support && spec && method)
        warmLang(root);
    return support && spec && method;
}

namespace {

const char *kInclude = "  include_context 'when the instance processes bot orders'";
const char *kPhrase = "when the instance processes bot orders";

void testParseContext()
{
    QString name;
    CHECK(rubyShared(QStringLiteral("shared_context 'when the instance processes bot orders' do"), &name));
    CHECK(name == QLatin1String(kPhrase));
    CHECK(!rubyShared(QStringLiteral("include_context 'when the instance processes bot orders'"), &name));
}

void testParseExamples()
{
    QString name;
    CHECK(rubyShared(QStringLiteral("shared_examples :logs do"), &name));
    CHECK(name == QLatin1String("logs"));
    CHECK(rubyShared(QStringLiteral("RSpec.shared_examples(\n  'rejects a request without service authentication'\n) do |verb, path|"), &name));
    CHECK(name == QLatin1String("rejects a request without service authentication"));
}

void checkContext(const QString &root)
{
    const int phrase = groupColumn(kInclude, kPhrase);
    CHECK(groupHit(root, 1, phrase, kPhrase, 4, "task_order"));
    const LangDef def = groupLookup(root, "spec/api/cancel_spec.rb", 1, phrase, kPhrase);
    CHECK(def.comment.contains(QLatin1String("bots")) && def.signature.contains(QLatin1String(kPhrase)));
    CHECK(groupHit(root, 1, groupColumn(kInclude, "include_context"), "include_context", 4, "task_order"));
}

void testContext()
{
    QTemporaryDir dir;
    const QString root = dir.path();
    CHECK(dir.isValid() && groupWarm(root));
    checkContext(root);
}

} // namespace

void rubyLikeTests();

int rubyGroupTests()
{
    testParseContext();
    testParseExamples();
    testContext();
    rubyLikeTests();
    return g_groupFails;
}
