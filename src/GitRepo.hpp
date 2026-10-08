#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

// What to ask git for.
enum class DiffMode { MergeRequest, Uncommitted, Staged };

// Result of one git diff load. `error` is empty on success.
struct GitResult {
    QString root;
    QString branch;
    QString baseRef;
    QString headRef;
    QStringList bases;
    QStringList branches;
    QString leftLabel;
    QString rightLabel;
    QString title;
    QString diffText;
    QString error;
    QString diskStamp;
    QString stamp;
    QStringList ignored;
    bool ignoredReady = false;
    bool unchanged = false;
};

// Runs git on a background thread and emits `ready` on the GUI thread.
class GitRepo : public QObject {
    Q_OBJECT
public:
    explicit GitRepo(QObject *parent = nullptr);
    void load(const QString &startPath, DiffMode mode, const QString &baseRef, const QString &headRef,
              const QString &stamp, bool quiet, bool listIgnored);

signals:
    void ready(const GitResult &result);

private:
    int generation = 0;
};
