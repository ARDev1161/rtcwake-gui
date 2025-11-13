#include "RtcWakeDaemon.h"

#include <QCoreApplication>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("rtcwake-daemon"));
    app.setOrganizationName(QStringLiteral("rtcwake-gui"));

    RtcWakeDaemon daemon;
    daemon.start();

    return app.exec();
}
