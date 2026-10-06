#include "Smoke.hpp"

#include "DiffCanvas.hpp"
#include "MainWindow.hpp"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QListWidget>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTimer>

namespace {

void clickGutter(DiffCanvas *canvas)
{
    canvas->setScrollTop(0);
    const QPoint pos(canvas->viewport()->width() / 2 + 10, 80);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(pos), QPointF(canvas->viewport()->mapToGlobal(pos)),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas->viewport(), &press);
}

void triggerCopy(MainWindow &window)
{
    for (QAction *action : window.actions()) {
        if (action->text() == QLatin1String("Copy reviews"))
            action->trigger();
    }
}

void logClip(const QString &clip, const QListWidget *list)
{
    qCritical().noquote() << QStringLiteral("clipboard:") << clip
                          << QStringLiteral("notes:") << (list ? list->count() : -1)
                          << QStringLiteral("row:") << (list ? list->currentRow() : -1);
}

void logEditor(QPlainTextEdit *editor, DiffCanvas *canvas)
{
    qCritical().noquote() << QStringLiteral("editor:") << (editor ? editor->toPlainText() : QStringLiteral("missing"))
                          << QStringLiteral("view:") << canvas->viewport()->size();
}

void logSmoke(MainWindow &window, QPlainTextEdit *editor, DiffCanvas *canvas, const QString &clip)
{
    logClip(clip, window.findChild<QListWidget *>());
    logEditor(editor, canvas);
}

bool commentsMatch(MainWindow &window, QPlainTextEdit *editor, DiffCanvas *canvas)
{
    const QString clip = QGuiApplication::clipboard()->text();
    if (clip.contains(QStringLiteral("src/app.cpp:")) && clip.contains(QStringLiteral("Check this.")))
        return true;
    logSmoke(window, editor, canvas, clip);
    return false;
}

bool checkComments(MainWindow &window)
{
    auto *canvas = window.findChild<DiffCanvas *>();
    if (!canvas)
        return false;
    clickGutter(canvas);
    auto *editor = window.findChild<QPlainTextEdit *>();
    if (editor)
        editor->setPlainText(QStringLiteral("Check this."));
    triggerCopy(window);
    return commentsMatch(window, editor, canvas);
}

void saveShot(MainWindow &window)
{
    const QString file = QDir::temp().filePath(QStringLiteral("hunk-smoke.png"));
    window.repaint();
    window.grab().save(file);
    qInfo().noquote() << file;
}

void finishSmoke(MainWindow &window, QApplication &app, bool ok)
{
    const QString previousClip = QGuiApplication::clipboard()->text();
    const bool commentsOk = !ok || checkComments(window);
    QGuiApplication::clipboard()->setText(previousClip);
    saveShot(window);
    app.exit(ok && commentsOk ? 0 : 1);
}

} // namespace

void scheduleSmoke(MainWindow &window, QApplication &app)
{
    QObject::connect(&window, &MainWindow::loaded, &app, [&window, &app](bool ok) {
        QTimer::singleShot(200, &app, [&window, &app, ok]() { finishSmoke(window, app, ok); });
    });
}
