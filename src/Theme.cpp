#include "MainDetail.hpp"

#include <QApplication>
#include <QPalette>

namespace {

const char *kStyle = R"(
QMainWindow, QWidget { background: #1e1e1e; color: #d4d4d4; }
QMenuBar, QMenu { background: #252526; color: #d4d4d4; }
QMenu::item:selected { background: #094771; }
QToolBar { background: #252526; border: none; spacing: 6px; padding: 4px; }
QToolButton, QPushButton { background: #2d2d2d; color: #d4d4d4; border: 1px solid #3c3c3c; padding: 4px 10px; }
QToolButton:hover, QPushButton:hover { background: #3c3c3c; }
QLineEdit, QPlainTextEdit, QComboBox, QListWidget, QTreeWidget {
  background: #1e1e1e; color: #d4d4d4; border: 1px solid #3c3c3c; padding: 4px;
  selection-background-color: #094771;
}
QComboBox QAbstractItemView { background: #2d2d2d; color: #d4d4d4; selection-background-color: #094771; }
QTreeWidget { border: none; padding: 0; }
QTreeWidget::item:selected { background: #094771; }
QSplitter::handle { background: #333333; }
QDockWidget { titlebar-close-icon: none; }
QDockWidget::title { background: #252526; padding: 6px; }
QStatusBar { background: #252526; color: #bbbbbb; }
QScrollBar:vertical { background: #1e1e1e; width: 12px; margin: 0; }
QScrollBar:horizontal { background: #1e1e1e; height: 12px; margin: 0; }
QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: #424242; border-radius: 4px; min-height: 24px; min-width: 24px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

void setRole(QPalette *palette, QPalette::ColorRole role, const char *hex)
{
    palette->setColor(role, QColor(QLatin1String(hex)));
}

} // namespace

void applyTheme()
{
    QPalette palette;
    setRole(&palette, QPalette::Window, "#1e1e1e");
    setRole(&palette, QPalette::WindowText, "#d4d4d4");
    setRole(&palette, QPalette::Base, "#1e1e1e");
    setRole(&palette, QPalette::Text, "#d4d4d4");
    setRole(&palette, QPalette::Button, "#2d2d2d");
    setRole(&palette, QPalette::ButtonText, "#d4d4d4");
    setRole(&palette, QPalette::Highlight, "#094771");
    palette.setColor(QPalette::HighlightedText, Qt::white);
    setRole(&palette, QPalette::PlaceholderText, "#858585");
    qApp->setPalette(palette);
    qApp->setStyleSheet(QString::fromUtf8(kStyle));
}
