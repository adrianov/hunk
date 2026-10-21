// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdHtml.hpp"

#include "DiffColors.hpp"

#include <QStringList>

namespace {

// The deepest block whose data-sourcepos holds a changed line is marked; a table cell marks
// its row. A line between a container's children, such as a blank one, marks nothing.
// Inside a code block only that line is tinted, under the text, unless its fence changed
// and the whole block is tinted. Marked blocks and lines are drawn as full-width bands
// behind the page, and neighbours merge across their margins.
const char *kMarks = R"(<style>
.markdown-body{position:relative;z-index:0;--hunk-tint:color-mix(in srgb,%1 %2%,transparent)}
.markdown-body tr.hunk-mark{background:transparent!important}
.markdown-body pre.hunk-mark{background-image:linear-gradient(var(--hunk-tint),var(--hunk-tint))}
.markdown-body .hunk-code{position:relative;z-index:0}
.hunk-line{position:absolute;left:0;min-width:100%;z-index:-1;background:var(--hunk-tint)}
.hunk-mark > .hunk-line{display:none}
.hunk-band{position:absolute;left:20px;right:20px;z-index:-1;border-radius:3px;
  background:var(--hunk-tint);box-shadow:inset 3px 0 %3}
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
for (const line of [%4]) {
  const hit = blocks.findLast(block => block.a <= line && line <= block.b);
  if (!hit || (line !== hit.a && hit.el.querySelector('[data-sourcepos]')))
    continue;
  const el = hit.el.matches('td, th') ? hit.el.parentElement : hit.el;
  const code = el.matches('pre') && el.querySelector('code:not(.language-mermaid)');
  if (!code || !codeLine(el, code, hit.a, line))
    el.classList.add('hunk-mark');
}
const layer = document.body.appendChild(document.createElement('div'));
const paint = () => {
  const top = document.body.getBoundingClientRect().top;
  const spans = [...document.querySelectorAll('.hunk-mark, .hunk-line')]
    .map(el => [el.getBoundingClientRect(), el.matches('.hunk-line') ? 0 : 4])
    .filter(([rect]) => rect.height > 0)
    .map(([rect, pad]) => [rect.top - top - pad, rect.bottom - top + pad])
    .sort((x, y) => x[0] - y[0]);
  const runs = [];
  for (const span of spans) {
    const last = runs.at(-1);
    if (last && span[0] - last[1] <= 24)
      last[1] = Math.max(last[1], span[1]);
    else
      runs.push(span);
  }
  layer.replaceChildren(...runs.map(([from, to]) => {
    const band = document.createElement('div');
    band.className = 'hunk-band';
    band.style.top = from + 'px';
    band.style.height = to - from + 'px';
    return band;
  }));
};
new ResizeObserver(paint).observe(document.body);
})();</script>)";

} // namespace

QString changeMarks(const MdSide &side)
{
    if (side.marks.isEmpty())
        return {};
    const QColor tint = side.added ? kAddBg : kDelBg;
    QStringList lines;
    for (int line : side.marks)
        lines.append(QString::number(line));
    return QString::fromLatin1(kMarks).arg(tint.name(), QString::number(qRound(tint.alphaF() * 100)),
                                           (side.added ? kAddFg : kDelFg).name(), lines.join(QLatin1Char(',')));
}
