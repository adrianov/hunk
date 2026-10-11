// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "LangDef.hpp"
#include "LangTune.hpp"

#include <QCursor>
#include <QGuiApplication>
#include <QScrollBar>

bool DiffCanvas::jumpMod(Qt::KeyboardModifiers mods) const
{
    // Command on macOS and Control on Linux both arrive as ControlModifier.
    return mods.testFlag(Qt::ControlModifier);
}

bool DiffCanvas::canFollow() const
{
    const QString name = langSeek(m_root, m_defPath, m_defLine, m_defName);
    if (name.isEmpty() || m_root.isEmpty() || !langSought(m_root, name))
        return false;
    return lookupDef(defSpot(m_root, m_defPath, m_defLine, m_defColumn, m_defName, m_defReceiver)).found();
}

void DiffCanvas::followHand()
{
    if (jumpMod(QGuiApplication::queryKeyboardModifiers()) && canFollow())
        viewport()->setCursor(Qt::PointingHandCursor);
}

void DiffCanvas::placeCursor()
{
    if (m_dragText || m_dragSplit)
        return;
    const QPoint local = viewport()->mapFromGlobal(QCursor::pos());
    if (!viewport()->rect().contains(local))
        return;
    const int y = local.y() + verticalScrollBar()->value();
    hoverCursor(bandAt(y), local.x());
    followHand();
}

void DiffCanvas::holdJump(const QString &name, const QString &path, int line, int column, const QString &receiver)
{
    m_jumpName = name;
    m_jumpPath = path;
    m_jumpLine = line;
    m_jumpColumn = column;
    m_jumpReceiver = receiver;
}

bool DiffCanvas::deferJump(const QString &name, const QString &path, int line, int column, const QString &receiver)
{
    const QString sought = langSeek(m_root, path, line, name);
    if (sought.isEmpty() || langSought(m_root, sought))
        return false;
    holdJump(name, path, line, column, receiver);
    seek(sought);
    return true;
}

void DiffCanvas::chooseDef(int x, int y)
{
    const TextMark hit = textAt(x, y);
    int line = 0;
    QString receiver;
    const QString name = hit.code ? methodName(hit.file, hit.row, hit.old, hit.pos, &line, &receiver) : QString();
    stopDef();
    if (name.isEmpty() || line <= 0 || m_root.isEmpty())
        return;
    const QString path = sidePath(m_doc.files.at(hit.file), hit.old);
    if (deferJump(name, path, line, hit.pos, receiver))
        return;
    const LangDef def = lookupDef(defSpot(m_root, path, line, hit.pos, name, receiver));
    if (def.found())
        emit openDefinition(def.path, def.line);
}

void DiffCanvas::finishJump()
{
    if (m_jumpName.isEmpty())
        return;
    const LangDef def = lookupDef(defSpot(m_root, m_jumpPath, m_jumpLine, m_jumpColumn, m_jumpName, m_jumpReceiver));
    m_jumpName.clear();
    if (def.found())
        emit openDefinition(def.path, def.line);
}
