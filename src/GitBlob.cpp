// Copyright © 2026 Peter Adrianov
// SPDX-License-Identifier: MIT

#include "GitDetail.hpp"

#include "DiffParse.hpp"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QProcess>

namespace {

QStringList splitBlob(const QString &text)
{
    if (text.isEmpty())
        return {};
    QStringList lines = text.split(QLatin1Char('\n'));
    if (lines.last().isEmpty())
        lines.removeLast();
    return lines;
}

QStringList worktreeLines(const QString &root, const QString &path)
{
    QFile file(QDir(root).filePath(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return splitBlob(QString::fromUtf8(file.readAll()));
}

int headerSize(const QByteArray &header, bool *missing)
{
    *missing = header.endsWith(" missing");
    if (*missing)
        return 0;
    const int space = header.lastIndexOf(' ');
    bool ok = false;
    const int size = space < 0 ? -1 : header.mid(space + 1).toInt(&ok);
    return ok && size >= 0 ? size : -1;
}

int copyBlob(const QByteArray &raw, int at, int size, QStringList *lines)
{
    if (size < 0 || at + size > raw.size())
        return -1;
    *lines = splitBlob(QString::fromUtf8(raw.mid(at, size)));
    at += size;
    if (at < raw.size() && raw.at(at) == '\n')
        ++at;
    return at;
}

int takeBlob(const QByteArray &raw, int at, QStringList *lines)
{
    const int nl = raw.indexOf('\n', at);
    if (nl < 0)
        return -1;
    bool missing = false;
    const int size = headerSize(raw.mid(at, nl - at), &missing);
    if (missing) {
        *lines = {};
        return nl + 1;
    }
    return copyBlob(raw, nl + 1, size, lines);
}

QByteArray blobSpec(const QString &rev, const QString &path)
{
    const QString name = rev == QLatin1String(":") ? QStringLiteral(":") : rev + QLatin1Char(':');
    return name.toUtf8() + path.toUtf8() + '\n';
}

QByteArray finishedBatch(QProcess *process)
{
    if (!process->waitForFinished(120000)) {
        process->kill();
        process->waitForFinished(2000);
        return {};
    }
    if (process->exitStatus() != QProcess::NormalExit || process->exitCode() != 0)
        return {};
    return process->readAllStandardOutput();
}

QByteArray gitBatch(const QString &root, const QByteArray &input)
{
    QProcess process;
    process.setProgram(QStringLiteral("git"));
    process.setArguments({QStringLiteral("-C"), root, QStringLiteral("cat-file"), QStringLiteral("--batch")});
    process.start();
    if (!process.waitForStarted(5000))
        return {};
    process.write(input);
    process.closeWriteChannel();
    return finishedBatch(&process);
}

QString blobKey(const QString &rev, const QString &path)
{
    return rev + QLatin1Char('\n') + path;
}

void noteAsk(QStringList *revs, QStringList *paths, const QString &rev, const QString &path)
{
    if (rev.isEmpty())
        return;
    revs->append(rev);
    paths->append(path);
}

QByteArray batchInput(const QStringList &revs, const QStringList &paths)
{
    QByteArray input;
    for (int index = 0; index < paths.size(); ++index)
        input += blobSpec(revs.at(index), paths.at(index));
    return input;
}

void storeBlobs(QHash<QString, QStringList> *lines, const QByteArray &raw, const QStringList &revs, const QStringList &paths)
{
    int at = 0;
    for (int index = 0; index < paths.size(); ++index) {
        QStringList text;
        at = takeBlob(raw, at, &text);
        if (at < 0)
            return;
        lines->insert(blobKey(revs.at(index), paths.at(index)), text);
    }
}

QHash<QString, QStringList> loadBlobs(const QString &root, const QStringList &revs, const QStringList &paths)
{
    QHash<QString, QStringList> lines;
    if (!paths.isEmpty())
        storeBlobs(&lines, gitBatch(root, batchInput(revs, paths)), revs, paths);
    return lines;
}

QStringList sideText(const QHash<QString, QStringList> &blobs, const QString &root, const QString &rev, const QString &path)
{
    if (rev.isEmpty())
        return worktreeLines(root, path);
    return blobs.value(blobKey(rev, path));
}

QString gapPath(const FileDiff &file, bool old)
{
    const QString path = old ? file.oldPath : file.newPath;
    if (path.isEmpty() || path == QLatin1String("/dev/null"))
        return file.path();
    return path;
}

void askFiles(QStringList *revs, QStringList *paths, const DiffDoc &doc, const GitResult &result)
{
    for (const FileDiff &file : doc.files) {
        if (file.binary || file.rows.isEmpty())
            continue;
        if (!file.added)
            noteAsk(revs, paths, result.leftRev, gapPath(file, true));
        if (!file.removed)
            noteAsk(revs, paths, result.rightRev, gapPath(file, false));
    }
}

void applyGaps(DiffDoc *doc, const GitResult &result, const QHash<QString, QStringList> &blobs)
{
    for (FileDiff &file : doc->files) {
        if (file.binary || file.rows.isEmpty())
            continue;
        const QStringList leftLines =
            file.added ? QStringList() : sideText(blobs, result.root, result.leftRev, gapPath(file, true));
        const QStringList rightLines =
            file.removed ? QStringList() : sideText(blobs, result.root, result.rightRev, gapPath(file, false));
        fillGaps(&file, leftLines, rightLines);
    }
}

} // namespace

void fillDocGaps(DiffDoc *doc, const GitResult &result)
{
    QStringList revs;
    QStringList paths;
    askFiles(&revs, &paths, *doc, result);
    applyGaps(doc, result, loadBlobs(result.root, revs, paths));
}
