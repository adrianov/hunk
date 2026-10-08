#include "MainWindow.hpp"
#include "Smoke.hpp"

#include <QApplication>

namespace {

void setupApp()
{
    QCoreApplication::setOrganizationName(QStringLiteral("Hunk"));
    QCoreApplication::setApplicationName(QStringLiteral("Hunk"));
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
