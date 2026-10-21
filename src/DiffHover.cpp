// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "DiffCanvas.hpp"

#include "DiffPopup.hpp"
#include "LangDef.hpp"
#include "LangTune.hpp"

#include <QCursor>
#include <QPointer>
#include <QThreadPool>

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

bool DiffCanvas::seekBusy(const QString &name) const
{
    return name.isEmpty() || m_root.isEmpty() || langSought(m_root, name) || m_seeking.contains(name);
}

void DiffCanvas::afterSeek(const QString &name)
{
    m_seeking.remove(name);
    if (m_defWait && langSeek(m_root, m_defPath, m_defLine, m_defName) == name && !m_defTimer.isActive())
        showDef();
    finishJump();
}

void DiffCanvas::seek(const QString &name)
{
    if (seekBusy(name))
        return;
    m_seeking.insert(name);
    const QString root = m_root;
    QPointer<DiffCanvas> alive(this);
    QThreadPool::globalInstance()->start([alive, root, name] {
        langEnsure(root, name);
        if (alive)
            QMetaObject::invokeMethod(alive.data(), [alive, name] { if (alive) alive->afterSeek(name); }, Qt::QueuedConnection);
    });
}

bool DiffCanvas::awaitSeek()
{
    const QString name = langSeek(m_root, m_defPath, m_defLine, m_defName);
    if (name.isEmpty() || langSought(m_root, name))
        return false;
    m_defWait = true;
    seek(name);
    return true;
}

void DiffCanvas::setRoot(const QString &root)
{
    if (root == m_root)
        return;
    m_root = root;
    m_seeking.clear();
    stopDef();
    if (root.isEmpty())
        return;
    QPointer<DiffCanvas> alive(this);
    warmLang(root, this, [alive] {
        if (!alive)
            return;
        if (alive->m_defWait && !alive->m_defTimer.isActive())
            alive->showDef();
        alive->finishJump();
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
    m_defReceiver.clear();
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
    m_defReceiver.clear();
    m_defWait = false;
    m_link.clear();
    m_jumpName.clear();
    m_jumpReceiver.clear();
    if (m_defPopup)
        m_defPopup->hide();
}

void DiffCanvas::keepDef(const QString &name, const QString &path, int line, int column, const QString &receiver)
{
    if (name == m_defName && path == m_defPath && line == m_defLine && receiver == m_defReceiver)
        return;
    m_defName = name;
    m_defPath = path;
    m_defLine = line;
    m_defColumn = column;
    m_defReceiver = receiver;
    if (m_defPopup)
        m_defPopup->hide();
    m_defTimer.start();
}

void DiffCanvas::hoverDef(int x, int y)
{
    const TextMark hit = textAt(x, y);
    m_link = linkPath(hit);
    int line = 0;
    QString receiver;
    const QString name = hit.code ? methodName(hit.file, hit.row, hit.old, hit.pos, &line, &receiver) : QString();
    if (name.isEmpty() || line <= 0 || m_root.isEmpty()) {
        dropDef();
        return;
    }
    keepDef(name, sidePath(m_doc.files.at(hit.file), hit.old), line, hit.pos, receiver);
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
    if (awaitSeek())
        return;
    const LangDef def = lookupDef(defSpot(m_root, m_defPath, m_defLine, m_defColumn, m_defName, m_defReceiver));
    m_defWait = false;
    if (def.found()) {
        ensurePopup()->present(def, m_mono, QCursor::pos(), m_defPath);
        followHand();
        return;
    }
    if (m_defPopup)
        m_defPopup->hide();
}

