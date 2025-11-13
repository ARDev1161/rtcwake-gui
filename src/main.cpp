#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("rtcwake-gui"));
    QCoreApplication::setApplicationName(QStringLiteral("planner"));
    MainWindow window;
    window.show();
    return app.exec();
}
