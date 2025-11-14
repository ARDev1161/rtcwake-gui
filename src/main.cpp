#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

#ifdef Q_OS_UNIX
#include <pwd.h>
#include <unistd.h>
#endif

namespace {
void ensureUserHomeEnv() {
#ifdef Q_OS_UNIX
    if (geteuid() != 0) {
        return;
    }
    const QByteArray sudoUser = qgetenv("SUDO_USER");
    if (sudoUser.isEmpty()) {
        return;
    }
    struct passwd *pwd = getpwnam(sudoUser.constData());
    if (!pwd || !pwd->pw_dir) {
        return;
    }
    const QByteArray targetHome = QByteArray(pwd->pw_dir);
    qputenv("HOME", targetHome);
    const QByteArray currentConfig = qgetenv("XDG_CONFIG_HOME");
    if (currentConfig.isEmpty() || currentConfig.startsWith("/root")) {
        QByteArray configHome = targetHome;
        if (!configHome.endsWith('/')) {
            configHome.append('/');
        }
        configHome.append(".config");
        qputenv("XDG_CONFIG_HOME", configHome);
    }
#endif
}
}

int main(int argc, char *argv[]) {
    ensureUserHomeEnv();
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("rtcwake-gui"));
    QCoreApplication::setApplicationName(QStringLiteral("planner"));
    MainWindow window;
    window.show();
    return app.exec();
}
