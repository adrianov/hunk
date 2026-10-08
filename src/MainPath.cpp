#include "MainWindow.hpp"

#include <QGuiApplication>
#include <QLabel>
#include <QSizePolicy>

void MainWindow::prepareRepoLabel()
{
    m_repoLabel->setMinimumWidth(0);
    m_repoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_repoLabel->installEventFilter(this);
    tintRepoLabel();
}

void MainWindow::tintRepoLabel()
{
    if (!m_repoLabel)
        return;
    const QColor text = qApp->palette().color(QPalette::Active, QPalette::Text);
    QPalette palette = m_repoLabel->palette();
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::ButtonText, text);
    m_repoLabel->setForegroundRole(QPalette::WindowText);
    m_repoLabel->setPalette(palette);
}

void MainWindow::showRepoPath()
{
    if (!m_repoLabel)
        return;
    tintRepoLabel();
    if (m_root.isEmpty())
        return;
    m_repoLabel->setToolTip(m_root);
    const int width = qMax(0, m_repoLabel->width() - 8);
    if (width < 12)
        return;
    const QString shown = m_repoLabel->fontMetrics().elidedText(m_root, Qt::ElideMiddle, width);
    if (m_repoLabel->text() != shown)
        m_repoLabel->setText(shown);
}
