// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace {

QString gitFile(const QString &root, const QString &dot)
{
    QFile file(dot);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QString line = QString::fromUtf8(file.readLine()).trimmed();
    const auto marker = QLatin1String("gitdir:");
    if (!line.startsWith(marker))
        return {};
    return QDir::cleanPath(QDir(root).absoluteFilePath(line.mid(marker.size()).trimmed()));
}

QString gitDir(const QString &root)
{
    const QString dot = QDir(root).filePath(QStringLiteral(".git"));
    const QFileInfo info(dot);
    if (info.isDir())
        return info.absoluteFilePath();
    return gitFile(root, dot);
}

QString fileMark(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists())
        return QStringLiteral("-");
    return QString::number(info.lastModified().toMSecsSinceEpoch()) + QLatin1Char('.') + QString::number(info.size());
}

} // namespace

QString refState(const QString &root)
{
    const QString git = gitDir(root);
    const QStringList tails{QStringLiteral("/packed-refs"), QStringLiteral("/refs"), QStringLiteral("/refs/heads"),
                            QStringLiteral("/refs/remotes/origin"), QStringLiteral("/refs/remotes/origin/HEAD")};
    QStringList marks;
    for (const QString &tail : tails)
        marks << fileMark(git + tail);
    return marks.join(QLatin1Char('|'));
}
