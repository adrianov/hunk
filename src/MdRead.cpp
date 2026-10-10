// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdRead.hpp"

#include "DiffColors.hpp"
#include "SyntaxRule.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTextBrowser>
#include <QUrl>

namespace {

QString sideDoc(const FileDiff &file, bool left)
{
    QStringList lines;
    for (const DiffRow &row : file.rows) {
        if ((left ? row.leftNum : row.rightNum) <= 0)
            continue;
        lines.append(left ? row.leftText : row.rightText);
    }
    return lines.join(QLatin1Char('\n'));
}

QString docDir(const QString &root, const FileDiff &file)
{
    return QFileInfo(QDir(root).filePath(file.path())).absolutePath();
}

QString mdCss()
{
    return QStringLiteral("p, li, h1, h2, h3, h4, h5, h6, pre, blockquote, td, th { color: %1; } a { color: %2; }")
        .arg(kText.name(), kAccent.name());
}

void tuneBrowser(QTextBrowser *view)
{
    view->setReadOnly(true);
    view->setOpenExternalLinks(true);
    view->setFrameShape(QFrame::NoFrame);
    view->document()->setDocumentMargin(18);
    QPalette palette = view->palette();
    palette.setColor(QPalette::Base, kBg);
    palette.setColor(QPalette::Text, kText);
    view->setPalette(palette);
}

} // namespace

bool markdownFile(const FileDiff &file)
{
    if (file.binary)
        return false;
    return langFor(file.path()) == Lang::Markdown || langFor(file.oldPath) == Lang::Markdown;
}

ReadText readFile(const FileDiff &file, const QString &root)
{
    ReadText text;
    text.dir = docDir(root, file);
    text.left = sideDoc(file, true);
    text.right = sideDoc(file, false);
    text.oldOnly = file.removed;
    text.pair = !file.removed && !file.singlePane();
    return text;
}

void fillBrowser(QTextBrowser *view, const QString &text, const QString &dir)
{
    tuneBrowser(view);
    QTextDocument *document = view->document();
    document->setDefaultStyleSheet(mdCss());
    const QString base = dir.endsWith(QLatin1Char('/')) ? dir : dir + QLatin1Char('/');
    document->setBaseUrl(QUrl::fromLocalFile(base));
    view->setMarkdown(text);
}

void tintWidget(QWidget *widget, const QColor &color)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::WindowText, color);
    palette.setColor(QPalette::Text, color);
    widget->setPalette(palette);
}

void paintGround(QWidget *widget)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, kBg);
    widget->setAutoFillBackground(true);
    widget->setPalette(palette);
}
