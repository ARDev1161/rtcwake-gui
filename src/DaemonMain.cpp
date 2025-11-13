#include "RtcWakeDaemon.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("rtcwake-daemon"));
    app.setOrganizationName(QStringLiteral("rtcwake-gui"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("rtcwake background scheduler"));
    parser.addHelpOption();

    QCommandLineOption configOpt(QStringLiteral("config"), QObject::tr("Path to the user config file"), QObject::tr("path"));
    QCommandLineOption userOpt(QStringLiteral("user"), QObject::tr("Target desktop user"), QObject::tr("name"));
    QCommandLineOption homeOpt(QStringLiteral("home"), QObject::tr("Target user home directory"), QObject::tr("dir"));
    QCommandLineOption warningOpt(QStringLiteral("warning-app"), QObject::tr("Path to the warning dialog executable"), QObject::tr("path"));

    parser.addOption(configOpt);
    parser.addOption(userOpt);
    parser.addOption(homeOpt);
    parser.addOption(warningOpt);

    parser.process(app);

    RtcWakeDaemon::Options options;
    options.configPath = parser.value(configOpt);
    options.targetUser = parser.value(userOpt);
    options.targetHome = parser.value(homeOpt);
    options.warningApp = parser.value(warningOpt);

    if (options.configPath.isEmpty() || options.targetUser.isEmpty() || options.targetHome.isEmpty() || options.warningApp.isEmpty()) {
        QTextStream(stderr) << QObject::tr("Missing required options. Use --help for details.\n");
        return 1;
    }

    RtcWakeDaemon daemon(options);
    daemon.start();

    return app.exec();
}
