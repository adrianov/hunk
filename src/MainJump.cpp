// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "MainSeen.hpp"
#include "MdView.hpp"

#include <QDir>
#include <QFile>

namespace {

QStringList plainLines(const QByteArray &bytes)
{
    QStringList lines = QString::fromUtf8(bytes).split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    return lines;
}

bool readPlain(const QString &root, const QString &path, FileDiff *file)
{
    QFile input(QDir(root).filePath(path));
    if (!input.open(QIODevice::ReadOnly))
        return false;
    const QByteArray bytes = input.readAll();
    if (bytes.contains('\0'))
        return false;
    *file = unchangedFile(path, plainLines(bytes));
    return true;
}

} // namespace

void MainWindow::keepOpened()
{
    QStringList still;
    for (const QString &path : m_opened) {
        if (fileNamed(m_doc, path) >= 0) {
            still.append(path);
            continue;
        }
        FileDiff plain;
        if (!readPlain(m_root, path, &plain))
            continue;
        m_doc.files.append(plain);
        still.append(path);
    }
    m_opened = still;
}

int MainWindow::plainIndex(const QString &path)
{
    if (path.isEmpty() || m_root.isEmpty())
        return -1;
    const int found = fileNamed(m_doc, path);
    if (found >= 0)
        return found;
    FileDiff plain;
    if (!readPlain(m_root, path, &plain))
        return -1;
    if (!m_opened.contains(path))
        m_opened.append(path);
    m_doc.files.append(plain);
    return m_doc.files.size() - 1;
}

void MainWindow::showPlain()
{
    rebuildTree();
    m_diff->setDoc(m_doc, m_leftLabel, m_rightLabel, hiddenIndexes(m_doc, m_hidden));
    m_md->reload(m_doc, m_root);
}

void MainWindow::showAt(int file, int line)
{
    bool oldSide = false;
    const int row = rowForLine(m_doc.files.at(file), line, &oldSide);
    if (row >= 0)
        m_diff->showRow(file, row, oldSide);
    else
        m_diff->showFile(file);
    selectTreeFile(file);
}

void MainWindow::openDefinition(const QString &path, int line)
{
    if (line <= 0)
        return;
    const int before = m_doc.files.size();
    const int file = plainIndex(path);
    if (file < 0)
        return;
    if (file >= before)
        showPlain();
    else if (fileHidden(m_doc.files.at(file), m_hidden))
        revealReviewed(file);
    showAt(file, line);
}
