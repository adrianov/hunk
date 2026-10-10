// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "MainWindow.hpp"

#ifdef Q_OS_MACOS
#include "MacIcon.hpp"
#endif

#include <QAction>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>

namespace {

QString aboutText()
{
    return QStringLiteral("<p><b>Hunk %1</b></p>"
                          "<p>Local merge-request diff for macOS and Linux.</p>"
                          "<p>Copyright © 2026 Peter Adrianov<br>MIT License</p>")
        .arg(QCoreApplication::applicationVersion());
}

QPixmap aboutIcon(const QWidget *widget)
{
    QPixmap icon(QStringLiteral(":/icons/hunk.png"));
#ifdef Q_OS_MACOS
    const QPixmap mac = macAppIcon();
    if (!mac.isNull())
        icon = mac;
#endif
    const qreal ratio = widget->devicePixelRatio();
    icon = icon.scaled(qRound(64 * ratio), qRound(64 * ratio), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    icon.setDevicePixelRatio(ratio);
    return icon;
}

#ifdef Q_OS_MACOS
QMessageBox *makeAbout(QWidget *parent)
{
    auto *box = new QMessageBox(QMessageBox::NoIcon, QStringLiteral("About Hunk"), aboutText(), QMessageBox::Ok, parent,
                                Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    box->setIconPixmap(aboutIcon(parent));
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setTextInteractionFlags(Qt::TextBrowserInteraction);
    if (auto *buttons = box->findChild<QDialogButtonBox *>())
        buttons->setCenterButtons(true);
    box->setModal(false);
    return box;
}

void raiseAbout(QMessageBox *box)
{
    box->show();
    box->raise();
    box->activateWindow();
}
#endif

} // namespace

void MainWindow::showAbout()
{
#ifdef Q_OS_MACOS
    static QPointer<QMessageBox> open;
    if (!open)
        open = makeAbout(this);
    raiseAbout(open);
#else
    QMessageBox box(QMessageBox::NoIcon, QStringLiteral("About Hunk"), aboutText(), QMessageBox::Ok, this);
    box.setIconPixmap(aboutIcon(this));
    box.setTextInteractionFlags(Qt::TextBrowserInteraction);
    box.exec();
#endif
}

void MainWindow::addAbout(QMenu *view)
{
#ifndef Q_OS_MACOS
    view = menuBar()->addMenu(QStringLiteral("Help"));
#endif
    auto *about = view->addAction(QStringLiteral("About Hunk"));
    about->setMenuRole(QAction::AboutRole);
    connect(about, &QAction::triggered, this, &MainWindow::showAbout);
}
