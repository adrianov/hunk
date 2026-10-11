// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffPopup.hpp"
#include "LangDef.hpp"
#include "SyntaxRule.hpp"

#include <QCursor>
#include <QPointer>
#include <QThreadPool>

namespace {

bool hoverKind(SynKind kind, const QString &path)
{
    if (kind == SynKind::Method)
        return true;
    return kind == SynKind::Variable && langFor(path) == Lang::Ruby;
}

QString spanName(const QString &text, const QList<SynSpan> &spans, int column, const QString &path)
{
    for (const SynSpan &span : spans) {
        if (column < span.start || column >= span.end || !hoverKind(span.kind, path))
            continue;
        return text.mid(span.start, span.end - span.start);
    }
    return {};
}

LangSpot defSpot(const QString &root, const QString &path, int line, int column, const QString &name)
{
    LangSpot spot;
    spot.root = root;
    spot.path = path;
    spot.line = line;
    spot.column = column;
    spot.name = name;
    return spot;
}

} // namespace

void DiffCanvas::armHover()
{
    m_defTimer.setSingleShot(true);
    m_defTimer.setInterval(280);
    connect(&m_defTimer, &QTimer::timeout, this, &DiffCanvas::showDef);
}

void DiffCanvas::onScroll()
{
    viewport()->update();
    emitVisibleFile();
    stopDef();
}

void DiffCanvas::setRoot(const QString &root)
{
    if (root == m_root)
        return;
    m_root = root;
    stopDef();
    if (root.isEmpty())
        return;
    QPointer<QObject> watch(this);
    QPointer<DiffCanvas> alive(this);
    QThreadPool::globalInstance()->start([watch, alive, root] {
        warmLang(root, watch, [alive] {
            if (!alive)
                return;
            if (alive->m_defWait && !alive->m_defTimer.isActive())
                alive->showDef();
            alive->finishJump();
        });
    });
}

bool DiffCanvas::overDef() const
{
    return m_defPopup && m_defPopup->isVisible() && m_defPopup->rect().contains(m_defPopup->mapFromGlobal(QCursor::pos()));
}

void DiffCanvas::dropDef()
{
    m_defName.clear();
    m_defPath.clear();
    m_defLine = 0;
    m_defWait = false;
    if (overDef())
        return;
    if (m_defPopup && m_defPopup->isVisible())
        m_defTimer.start();
    else
        m_defTimer.stop();
}

void DiffCanvas::stopDef()
{
    m_defTimer.stop();
    m_defName.clear();
    m_defPath.clear();
    m_defLine = 0;
    m_defWait = false;
    m_jumpName.clear();
    if (m_defPopup)
        m_defPopup->hide();
}

QString DiffCanvas::methodName(int file, int row, bool old, int column, int *line) const
{
    const FileDiff &fileDiff = m_doc.files.at(file);
    const DiffRow &lineRow = fileDiff.rows.at(row);
    const QString path = sidePath(fileDiff, old);
    *line = old ? lineRow.leftNum : lineRow.rightNum;
    return spanName(old ? lineRow.leftText : lineRow.rightText, old ? lineRow.leftSyn : lineRow.rightSyn, column, path);
}

void DiffCanvas::hoverDef(int x, int y)
{
    const TextMark hit = textAt(x, y);
    int line = 0;
    const QString name = hit.code ? methodName(hit.file, hit.row, hit.old, hit.pos, &line) : QString();
    if (name.isEmpty() || line <= 0 || m_root.isEmpty()) {
        dropDef();
        return;
    }
    const QString path = sidePath(m_doc.files.at(hit.file), hit.old);
    if (name == m_defName && path == m_defPath && line == m_defLine)
        return;
    m_defName = name;
    m_defPath = path;
    m_defLine = line;
    m_defColumn = hit.pos;
    if (m_defPopup)
        m_defPopup->hide();
    m_defTimer.start();
}

DefPopup *DiffCanvas::ensurePopup()
{
    if (m_defPopup)
        return static_cast<DefPopup *>(m_defPopup);
    auto *popup = new DefPopup(this);
    popup->onLeave = [this] {
        if (!rect().contains(mapFromGlobal(QCursor::pos())))
            stopDef();
    };
    m_defPopup = popup;
    return popup;
}

void DiffCanvas::showDef()
{
    if (overDef())
        return;
    if (m_defName.isEmpty() || m_root.isEmpty()) {
        m_defWait = false;
        if (m_defPopup)
            m_defPopup->hide();
        return;
    }
    const LangDef def = lookupDef(defSpot(m_root, m_defPath, m_defLine, m_defColumn, m_defName));
    if (def.found()) {
        m_defWait = false;
        ensurePopup()->present(def, m_mono, QCursor::pos());
        return;
    }
    if (!langReady(m_root, m_defPath)) {
        m_defWait = true;
        return;
    }
    m_defWait = false;
    if (m_defPopup)
        m_defPopup->hide();
}

bool DiffCanvas::jumpMod(Qt::KeyboardModifiers mods) const
{
#ifdef Q_OS_MACOS
    return mods.testFlag(Qt::MetaModifier);
#else
    return mods.testFlag(Qt::ControlModifier);
#endif
}

void DiffCanvas::holdJump(const QString &name, const QString &path, int line, int column)
{
    m_jumpName = name;
    m_jumpPath = path;
    m_jumpLine = line;
    m_jumpColumn = column;
}

void DiffCanvas::chooseDef(int x, int y)
{
    const TextMark hit = textAt(x, y);
    int line = 0;
    const QString name = hit.code ? methodName(hit.file, hit.row, hit.old, hit.pos, &line) : QString();
    stopDef();
    if (name.isEmpty() || line <= 0 || m_root.isEmpty())
        return;
    const QString path = sidePath(m_doc.files.at(hit.file), hit.old);
    const LangDef def = lookupDef(defSpot(m_root, path, line, hit.pos, name));
    if (def.found())
        emit openDefinition(def.path, def.line);
    else if (!langReady(m_root, path))
        holdJump(name, path, line, hit.pos);
}

void DiffCanvas::finishJump()
{
    if (m_jumpName.isEmpty())
        return;
    const LangDef def = lookupDef(defSpot(m_root, m_jumpPath, m_jumpLine, m_jumpColumn, m_jumpName));
    m_jumpName.clear();
    if (def.found())
        emit openDefinition(def.path, def.line);
}
