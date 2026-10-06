#pragma once

#include <QString>
#include <QStringList>

struct GitCmd {
    int code = -1;
    QString out;
    QString err;
};

GitCmd runGit(const QString &cwd, const QStringList &args);
QStringList baseRefs(const QString &root);
