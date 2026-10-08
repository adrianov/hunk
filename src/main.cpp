// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"
#include "Smoke.hpp"

#include <QApplication>
#include <QIcon>

#ifdef Q_OS_MACOS
#include "MacIcon.hpp"
#endif

namespace {

void setupApp()
{
    QCoreApplication::setOrganizationName(QStringLiteral("Hunk"));
    QCoreApplication::setApplicationName(QStringLiteral("Hunk"));
    QCoreApplication::setApplicationVersion(QStringLiteral(HUNK_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("hunk"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/hunk.png")));
#ifdef Q_OS_MACOS
    applyMacIcon();
#endif
}

QString launchPath(bool *smoke)
{
    QString path;
    const QStringList args = QCoreApplication::arguments();
    for (int index = 1; index < args.size(); ++index) {
        if (args.at(index) == QLatin1String("--smoke"))
            *smoke = true;
        else if (!args.at(index).startsWith(QLatin1Char('-')))
            path = args.at(index);
    }
    return path;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    setupApp();
    bool smoke = false;
    const QString path = launchPath(&smoke);
    MainWindow window(smoke);
    window.resize(1280, 800);
    window.show();
    if (smoke)
        scheduleSmoke(window, app);
    window.openStart(path);
    return app.exec();
}
