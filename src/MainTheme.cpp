#include "MainWindow.hpp"

#include "DiffCanvas.hpp"
#include "MainDetail.hpp"

#include <QGuiApplication>
#include <QStyleHints>

void MainWindow::watchSystemTheme()
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (m_smoke || themePick() != ThemePick::System)
            return;
        noteSystemTheme();
        repaintTheme();
    });
}

void MainWindow::repaintTheme()
{
    applyTheme(m_smoke);
    if (!m_diff)
        return;
    m_diff->update();
    showRepoPath();
    rebuildTree();
    refreshNotes();
}

void MainWindow::chooseTheme(ThemePick pick)
{
    if (!m_smoke)
        saveThemePick(pick);
    repaintTheme();
}
