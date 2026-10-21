// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdHtml.hpp"

#include "DiffColors.hpp"

#include <QStringList>

namespace {

// The deepest block whose data-sourcepos holds a changed line gets tinted; a table cell
// tints its row. Inside a code block only that line is tinted, under the text.
const char *kMarks = R"(<style>
.markdown-body{--hunk-fill:color-mix(in srgb,%1 %2%,%3);--hunk-line:color-mix(in srgb,%1 %2%,transparent)}
.markdown-body .hunk-mark{background:var(--hunk-fill)!important;border-radius:2px;
  box-shadow:0 0 0 6px var(--hunk-fill),-4px 0 0 6px %4}
.markdown-body tr.hunk-mark{box-shadow:none}
.markdown-body .hunk-code{position:relative;z-index:0}
.hunk-line{position:absolute;left:0;z-index:-1;background:var(--hunk-line);box-shadow:inset 3px 0 %4}
</style><script>(() => {
const blocks = [...document.querySelectorAll('[data-sourcepos]')].map(el => {
  const [a, b] = el.dataset.sourcepos.split('-').map(pos => parseInt(pos));
  return {el, a, b};
});
const codeLine = (pre, code, a, line) => {
  const count = code.textContent.split('\n').length - 1;
  const index = line - a - (pre.hasAttribute('data-fence') ? 1 : 0);
  if (index < 0 || index >= count)
    return false;
  const style = getComputedStyle(pre);
  const height = parseFloat(style.lineHeight);
  const bar = document.createElement('div');
  bar.className = 'hunk-line';
  bar.style.top = parseFloat(style.paddingTop) + index * height + 'px';
  bar.style.height = height + 'px';
  bar.style.width = pre.scrollWidth + 'px';
  pre.classList.add('hunk-code');
  pre.appendChild(bar);
  return true;
};
for (const line of [%5]) {
  const hit = blocks.findLast(block => block.a <= line && line <= block.b);
  if (!hit)
    continue;
  const el = hit.el.matches('td, th') ? hit.el.parentElement : hit.el;
  const code = el.matches('pre') && el.querySelector('code:not(.language-mermaid)');
  if (!code || !codeLine(el, code, hit.a, line))
    el.classList.add('hunk-mark');
}
})();</script>)";

} // namespace

QString changeMarks(const MdSide &side, const QColor &ground)
{
    if (side.marks.isEmpty())
        return {};
    const QColor tint = side.added ? kAddBg : kDelBg;
    QStringList lines;
    for (int line : side.marks)
        lines.append(QString::number(line));
    return QString::fromLatin1(kMarks).arg(tint.name(), QString::number(qRound(tint.alphaF() * 100)), ground.name(),
                                           (side.added ? kAddFg : kDelFg).name(), lines.join(QLatin1Char(',')));
}
