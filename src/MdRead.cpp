// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdRead.hpp"

#include "DiffColors.hpp"
#include "MdHtml.hpp"
#include "SyntaxRule.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QWebEngineView>

namespace {

// Changed lines are marked only when both sides are shown.
MdSide sideDoc(const FileDiff &file, bool left, bool pair)
{
    MdSide side;
    side.added = !left;
    QStringList lines;
    for (const DiffRow &row : file.rows) {
        if ((left ? row.leftNum : row.rightNum) <= 0)
            continue;
        lines.append(left ? row.leftText : row.rightText);
        if (pair && row.kind != RowKind::Context)
            side.marks.append(lines.size());
    }
    side.text = lines.join(QLatin1Char('\n'));
    return side;
}

QString docDir(const QString &root, const FileDiff &file)
{
    return QFileInfo(QDir(root).filePath(file.path())).absolutePath();
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
    text.oldOnly = file.removed;
    text.pair = !file.removed && !file.singlePane();
    text.left = sideDoc(file, true, text.pair);
    text.right = sideDoc(file, false, text.pair);
    return text;
}

void fillBrowser(QWebEngineView *view, const MdSide &side, const QString &dir)
{
    showMarkdown(view, side, dir, kBg.lightness() < 128);
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
