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

// Tags each fenced block: its first source line is the fence, so its code starts one line lower.
QString fencedHtml(cmark_node *doc, const char *rendered)
{
    QString html = QString::fromUtf8(rendered ? rendered : "");
    cmark_iter *iter = cmark_iter_new(doc);
    while (cmark_iter_next(iter) != CMARK_EVENT_DONE) {
        cmark_node *node = cmark_iter_get_node(iter);
        int size = 0;
        char fence = 0;
        if (!cmark_node_get_fenced(node, &size, &size, &fence))
            continue;
        const QString pos = QStringLiteral(" data-sourcepos=\"%1:").arg(cmark_node_get_start_line(node));
        html.replace(QStringLiteral("<pre") + pos, QStringLiteral("<pre data-fence") + pos);
    }
    cmark_iter_free(iter);
    return html;
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
    char *html = cmark_render_html(doc, CMARK_OPT_SOURCEPOS, cmark_parser_get_syntax_extensions(parser));
    const QString body = fencedHtml(doc, html);
    free(html);
    cmark_node_free(doc);
    cmark_parser_free(parser);
    return body;
}

// Turns ```mermaid blocks into diagrams; runs before highlight.js would color them as code.
// Mermaid is over the 2 MB setHtml limit, so it loads from the resource instead of inline.
QString mermaidScript(bool dark)
{
    return QStringLiteral("<script src=\"qrc:/md/mermaid.js\"></script><script>"
                          "for (const code of document.querySelectorAll('pre > code.language-mermaid')) {"
                          "const box = document.createElement('div');"
                          "box.className = code.parentNode.className + ' mermaid';"
                          "box.textContent = code.textContent;"
                          "code.parentNode.replaceWith(box);"
                          "}"
                          "mermaid.initialize({startOnLoad: false, theme: '%1'});"
                          "mermaid.run();"
                          "</script>")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("default"));
}

QString pageFor(const QString &body, const QString &marks, bool dark)
{
    QString page;
    page += QStringLiteral("<!doctype html><html><head><meta charset=\"utf-8\"><style>");
    page += asset(dark ? ":/md/github-dark.css" : ":/md/github-light.css");
    page += QStringLiteral("</style><style>");
    page += asset(dark ? ":/md/hl-dark.css" : ":/md/hl-light.css");
    page += QStringLiteral("</style><style>.markdown-body{box-sizing:border-box;padding:28px 36px;}</style></head>"
                           "<body class=\"markdown-body\">");
    page += body;
    page += marks;
    page += QStringLiteral("<script>");
    page += asset(":/md/highlight.js");
    page += QStringLiteral("</script>");
    if (body.contains(QLatin1String("language-mermaid")))
        page += mermaidScript(dark);
    page += QStringLiteral("<script>hljs.highlightAll();</script></body></html>");
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

void showMarkdown(QWebEngineView *view, const MdSide &side, const QString &dir, bool dark)
{
    const QString base = dir.endsWith(QLatin1Char('/')) ? dir : dir + QLatin1Char('/');
    const QColor ground = dark ? QColor(0x0d, 0x11, 0x17) : QColor(Qt::white);
    view->page()->setBackgroundColor(ground);
    view->setHtml(pageFor(gfmBody(side.text), changeMarks(side, ground), dark), QUrl::fromLocalFile(base));
}
