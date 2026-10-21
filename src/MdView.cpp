// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MdView.hpp"

#include "DiffCanvas.hpp"
#include "DiffColors.hpp"
#include "MdRead.hpp"

#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QWebEngineView>
#include <QVBoxLayout>

MdView::MdView(DiffCanvas *canvas, QWidget *parent)
    : QWidget(parent)
    , m_canvas(canvas)
{
    buildBar();
    buildPage();
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_canvas);
    m_stack->addWidget(m_page);
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(m_bar);
    lay->addWidget(m_stack, 1);
    wireSwitch();
    applyTheme();
}

void MdView::setLabels(const QString &left, const QString &right)
{
    m_leftName = left;
    m_rightName = right;
}

void MdView::leaveFile(int file)
{
    if (m_file != file)
        return;
    m_file = -1;
    apply();
}

const FileDiff *MdView::currentFile() const
{
    if (!m_doc || m_file < 0 || m_file >= m_doc->files.size())
        return nullptr;
    return &m_doc->files.at(m_file);
}

void MdView::choose(int id)
{
    const bool on = id == 1;
    if (on == m_on || !m_doc)
        return;
    m_on = on;
    apply();
    if (m_on && m_left)
        (m_rightPane->isVisible() ? m_right : m_left)->setFocus();
}

void MdView::showDocs(const FileDiff &file)
{
    m_text = readFile(file, m_root);
    ensureBrowser(&m_left, m_leftPane);
    ensureBrowser(&m_right, m_rightPane);
    fillBrowser(m_left, m_text.left, m_text.dir);
    fillBrowser(m_right, m_text.right, m_text.dir);
    m_leftPane->setVisible(m_text.pair || m_text.oldOnly);
    m_rightPane->setVisible(!m_text.oldOnly);
    m_leftCaption->setText(m_leftName);
    m_leftCaption->setToolTip(m_leftName);
    m_rightCaption->setText(m_rightName);
    m_rightCaption->setToolTip(m_rightName);
}

void MdView::apply()
{
    const FileDiff *file = currentFile();
    const bool md = file && markdownFile(*file);
    m_bar->setVisible(md);
    if (md)
        showTitle(*file);
    if (md && m_on) {
        showDocs(*file);
        m_stack->setCurrentWidget(m_page);
    } else {
        m_stack->setCurrentWidget(m_canvas);
    }
    m_canvas->barFile(md ? m_file : -1);
}

void MdView::showTitle(const FileDiff &file)
{
    m_title->setText(file.title());
    m_title->setToolTip(file.title());
    m_counts->setText(QStringLiteral("<span style=\"color:%1\">+%2</span>  <span style=\"color:%3\">−%4</span>")
                          .arg(kAddFg.name(), QString::number(file.adds), kDelFg.name(), QString::number(file.dels)));
}

void MdView::track(int file, const DiffDoc &doc, const QString &root)
{
    m_file = file;
    m_doc = &doc;
    m_root = root;
    apply();
}

void MdView::reload(const DiffDoc &doc, const QString &root)
{
    if (m_file >= doc.files.size())
        m_file = -1;
    track(m_file, doc, root);
}

void MdView::showSource()
{
    if (!m_on)
        return;
    m_on = false;
    m_modes->blockSignals(true);
    m_source->setChecked(true);
    m_modes->blockSignals(false);
    apply();
}

void MdView::applyTheme()
{
    m_bar->update();
    tintWidget(m_title, kFile);
    tintWidget(m_leftCaption, kMuted);
    tintWidget(m_rightCaption, kMuted);
    paintGround(m_page);
    if (m_left)
        fillBrowser(m_left, m_text.left, m_text.dir);
    if (m_right)
        fillBrowser(m_right, m_text.right, m_text.dir);
}
