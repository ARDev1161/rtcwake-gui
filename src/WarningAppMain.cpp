#include "WarningBanner.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTextStream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("rtcwake-warning"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("rtcwake warning dialog"));
    parser.addHelpOption();

    QCommandLineOption messageOpt(QStringList{QStringLiteral("m"), QStringLiteral("message")}, QObject::tr("Banner message"), QObject::tr("text"), QObject::tr("System will suspend soon. Save your work."));
    QCommandLineOption countdownOpt(QStringList{QStringLiteral("c"), QStringLiteral("countdown")}, QObject::tr("Countdown seconds"), QObject::tr("seconds"), QStringLiteral("30"));
    QCommandLineOption snoozeOpt(QStringList{QStringLiteral("s"), QStringLiteral("snooze")}, QObject::tr("Snooze minutes"), QObject::tr("minutes"), QStringLiteral("5"));
    QCommandLineOption actionOpt(QStringList{QStringLiteral("a"), QStringLiteral("action")}, QObject::tr("Action label"), QObject::tr("label"), QObject::tr("Suspend"));

    parser.addOption(messageOpt);
    parser.addOption(countdownOpt);
    parser.addOption(snoozeOpt);
    parser.addOption(actionOpt);

    parser.process(app);

    const QString message = parser.value(messageOpt);
    const int countdownSeconds = parser.value(countdownOpt).toInt();
    const int snoozeMinutes = parser.value(snoozeOpt).toInt();
    const QString actionLabel = parser.value(actionOpt);

    WarningBanner banner(QObject::tr("%1\nAction: %2").arg(message, actionLabel), countdownSeconds);
    const auto result = banner.execWithCountdown();

    switch (result) {
    case WarningBanner::Result::ApplyNow:
        return 0;
    case WarningBanner::Result::Postpone:
        Q_UNUSED(snoozeMinutes);
        return 1;
    case WarningBanner::Result::Cancelled:
    default:
        return 2;
    }
}
