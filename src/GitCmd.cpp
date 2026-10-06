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

QString upstreamName(const QString &root)
{
    const GitCmd upstream = runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"), QStringLiteral("@{u}")});
    if (upstream.code != 0)
        return {};
    const QString name = upstream.out.trimmed();
    if (name.isEmpty() || name == QLatin1String("@{u}"))
        return {};
    return name;
}

bool refExists(const QString &root, const QString &name)
{
    return runGit(root, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), QStringLiteral("--quiet"), name}).code == 0;
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

QStringList baseRefs(const QString &root)
{
    QStringList bases;
    const QString upstream = upstreamName(root);
    if (!upstream.isEmpty())
        bases << upstream;
    addKnownBases(root, &bases);
    return bases;
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
