// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdHtml.hpp"

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm-extension_api.h>
#include <cmark-gfm.h>

#include <QColor>
#include <QDesktopServices>
#include <QFile>
#include <QHash>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineView>

#include <cstdlib>

namespace {

class MdPage : public QWebEnginePage {
public:
    using QWebEnginePage::QWebEnginePage;

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool main) override
    {
        if (type != NavigationTypeLinkClicked)
            return QWebEnginePage::acceptNavigationRequest(url, type, main);
        if (url.hasFragment() && url.adjusted(QUrl::RemoveFragment) == this->url().adjusted(QUrl::RemoveFragment))
            return true;
        QDesktopServices::openUrl(url);
        return false;
    }
};

QString asset(const char *path)
{
    static QHash<QByteArray, QString> cache;
    const QByteArray key(path);
    if (cache.contains(key))
        return cache.value(key);
    QFile file(QString::fromLatin1(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QString text = QString::fromUtf8(file.readAll());
    cache.insert(key, text);
    return text;
}

void useExt(cmark_parser *parser, const char *name)
{
    cmark_syntax_extension *syntax = cmark_find_syntax_extension(name);
    if (syntax)
        cmark_parser_attach_syntax_extension(parser, syntax);
}

QString gfmBody(const QString &markdown)
{
    const QByteArray utf8 = markdown.toUtf8();
    cmark_gfm_core_extensions_ensure_registered();
    cmark_parser *parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    static const char *names[] = {"table", "strikethrough", "autolink", "tagfilter", "tasklist"};
    for (const char *name : names)
        useExt(parser, name);
    cmark_parser_feed(parser, utf8.constData(), static_cast<size_t>(utf8.size()));
    cmark_node *doc = cmark_parser_finish(parser);
    char *html = cmark_render_html(doc, CMARK_OPT_DEFAULT, cmark_parser_get_syntax_extensions(parser));
    const QString body = QString::fromUtf8(html ? html : "");
    free(html);
    cmark_node_free(doc);
    cmark_parser_free(parser);
    return body;
}

QString pageFor(const QString &body, bool dark)
{
    QString page;
    page += QStringLiteral("<!doctype html><html><head><meta charset=\"utf-8\"><style>");
    page += asset(dark ? ":/md/github-dark.css" : ":/md/github-light.css");
    page += QStringLiteral("</style><style>");
    page += asset(dark ? ":/md/hl-dark.css" : ":/md/hl-light.css");
    page += QStringLiteral("</style><style>.markdown-body{box-sizing:border-box;padding:28px 36px;}</style></head>"
                           "<body class=\"markdown-body\">");
    page += body;
    page += QStringLiteral("<script>");
    page += asset(":/md/highlight.js");
    page += QStringLiteral("</script><script>hljs.highlightAll();</script></body></html>");
    return page;
}

} // namespace

void prepareMarkdown(QWebEngineView *view)
{
    auto *page = new MdPage(view);
    QWebEngineSettings *settings = page->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    view->setPage(page);
}

void showMarkdown(QWebEngineView *view, const QString &markdown, const QString &dir, bool dark)
{
    const QString base = dir.endsWith(QLatin1Char('/')) ? dir : dir + QLatin1Char('/');
    view->page()->setBackgroundColor(dark ? QColor(0x0d, 0x11, 0x17) : QColor(Qt::white));
    view->setHtml(pageFor(gfmBody(markdown), dark), QUrl::fromLocalFile(base));
}
