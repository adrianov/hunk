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

LangDef lookupCall(const QString &root, const char *path, const char *receiver, const char *name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = QLatin1String(path);
    spot.receiver = QLatin1String(receiver);
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

bool forwardFiles(const QString &root)
{
    return putFile(root, "app/models/order.rb",
                   "class Order\n  def cart_items_price = Carts::Items.price(online_order: self)\n"
                   "  def bag = items.bag unless packed\nend\n")
        && putFile(root, "app/services/carts/items.rb",
                   "class Carts::Items\n  def self.price(...) = new(...).price\n\n  # Sum of the lines\n"
                   "  def price\n    total\n  end\n  def self.plain = 1\nend\n")
        && putFile(root, "call.rb", "Carts::Items.price\n");
}

bool bodyHas(const LangDef &def, const char *bit)
{
    return def.body.contains(QLatin1String(bit));
}

bool stopped(const LangDef &def, const char *sig)
{
    return def.signature.contains(QLatin1String(sig)) && def.body.isEmpty();
}

void testForwardPrice()
{
    QTemporaryDir dir;
    CHECK(dir.isValid() && forwardFiles(dir.path()));
    warmLang(dir.path());
    const LangDef price = lookupCall(dir.path(), "call.rb", "Carts::Items", "price");
    CHECK(price.signature.contains(QLatin1String("def self.price")) && bodyHas(price, "def price")
          && bodyHas(price, "# Sum of the lines") && !bodyHas(price, "items.rb"));
}

void testForwardChain()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(forwardFiles(root));
    warmLang(root);
    const LangDef cart = lookupCall(root, "call.rb", "Order", "cart_items_price");
    CHECK(cart.signature.contains(QLatin1String("def cart_items_price")));
    CHECK(bodyHas(cart, "def self.price") && bodyHas(cart, "total") && bodyHas(cart, "items.rb:"));
}

void testForwardStop()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString root = dir.path();
    CHECK(forwardFiles(root));
    warmLang(root);
    CHECK(stopped(lookupCall(root, "call.rb", "Order", "bag"), "unless packed"));
    CHECK(stopped(lookupCall(root, "call.rb", "Carts::Items", "plain"), "= 1"));
}

} // namespace

int rubyForwardTests()
{
    testForwardPrice();
    testForwardChain();
    testForwardStop();
    return g_fails;
}
