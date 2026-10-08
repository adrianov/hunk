#include "GitDetail.hpp"

#include <QProcess>
#include <QProcessEnvironment>

namespace {

QStringList gitConfig()
{
    return {QStringLiteral("-c"), QStringLiteral("color.ui=false"),
            QStringLiteral("-c"), QStringLiteral("color.diff=false"),
            QStringLiteral("-c"), QStringLiteral("core.quotepath=false")};
}

QProcessEnvironment gitEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GIT_PAGER"), QString());
    env.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    return env;
}

void launchGit(QProcess &process, const QString &cwd, const QStringList &args)
{
    QStringList full = gitConfig();
    if (!cwd.isEmpty())
        full << QStringLiteral("-C") << cwd;
    full << args;
    process.setProgram(QStringLiteral("git"));
    process.setArguments(full);
    process.setProcessEnvironment(gitEnv());
    process.start();
}

GitCmd readOutput(QProcess &process)
{
    GitCmd cmd;
    cmd.code = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    cmd.out = QString::fromUtf8(process.readAllStandardOutput());
    cmd.err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return cmd;
}

GitCmd stopGit(QProcess &process, const QString &message)
{
    process.kill();
    process.waitForFinished(2000);
    return {-1, {}, message};
}

QString upstreamOf(const QString &root, const QString &ref)
{
    const GitCmd upstream = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"),
                                          ref + QStringLiteral("@{upstream}")});
    if (upstream.code != 0)
        return {};
    const QString name = upstream.out.trimmed();
    if (name.isEmpty() || name.contains(QLatin1String("@{")))
        return {};
    return name;
}

bool refExists(const QString &root, const QString &name)
{
    return runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), name}).code == 0;
}

QString originHead(const QString &root)
{
    const GitCmd origin = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"),
                                        QStringLiteral("origin/HEAD")});
    if (origin.code != 0)
        return {};
    const QString name = origin.out.trimmed();
    if (name.isEmpty() || name.contains(QLatin1String("@{")))
        return {};
    return name;
}

QString defaultBase(const QString &root)
{
    const QString origin = originHead(root);
    return origin.isEmpty() ? baseRefs(root).value(0) : origin;
}

void addKnownBases(const QString &root, QStringList *bases)
{
    const QStringList candidates{QStringLiteral("origin/main"), QStringLiteral("main"),
                                 QStringLiteral("origin/master"), QStringLiteral("master")};
    for (const QString &candidate : candidates) {
        if (!bases->contains(candidate) && refExists(root, candidate))
            bases->push_back(candidate);
    }
}

} // namespace

QString branchPointName()
{
    return QStringLiteral("branching point");
}

QStringList baseRefs(const QString &root)
{
    QStringList bases;
    const QString upstream = upstreamOf(root, QStringLiteral("HEAD"));
    if (!upstream.isEmpty())
        bases << upstream;
    addKnownBases(root, &bases);
    return bases;
}

QStringList baseChoices(const QString &root)
{
    QStringList bases{branchPointName()};
    for (const QString &name : baseRefs(root)) {
        if (!bases.contains(name))
            bases << name;
    }
    return bases;
}

QStringList branchRefs(const QString &root, const QString &current)
{
    QStringList names = runGit(root, {QStringLiteral("for-each-ref"), QStringLiteral("--format=%(refname:short)"),
                                      QStringLiteral("refs/heads")})
                            .out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (!current.isEmpty()) {
        names.removeAll(current);
        names.prepend(current);
    }
    return names;
}

QString branchPoint(const QString &root, const QString &headRef)
{
    const QString parent = defaultBase(root);
    if (parent.isEmpty())
        return {};
    const GitCmd base = runGit(root, {QStringLiteral("merge-base"), parent, headRef});
    return base.code == 0 ? base.out.trimmed() : QString();
}

QString workStamp(const QString &root)
{
    const GitCmd head = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
    if (head.code != 0)
        return {};
    const GitCmd status = runGit(root, {QStringLiteral("status"), QStringLiteral("--porcelain"), QStringLiteral("-uno")});
    if (status.code != 0)
        return {};
    return head.out.trimmed() + QLatin1Char('\n') + status.out;
}

GitCmd runGit(const QString &cwd, const QStringList &args)
{
    QProcess process;
    launchGit(process, cwd, args);
    if (!process.waitForStarted(5000))
        return {-1, {}, QStringLiteral("git not found")};
    if (!process.waitForFinished(120000))
        return stopGit(process, QStringLiteral("git timed out"));
    return readOutput(process);
}
