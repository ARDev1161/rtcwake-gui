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
    QCommandLineOption soundEnabledOpt(QStringList{QStringLiteral("sound-enabled")}, QObject::tr("Enable sound alert"));
    QCommandLineOption soundFileOpt(QStringList{QStringLiteral("sound-file")}, QObject::tr("Sound file path"), QObject::tr("path"));
    QCommandLineOption volumeOpt(QStringList{QStringLiteral("v"), QStringLiteral("volume")}, QObject::tr("Sound volume (0-100)"), QObject::tr("volume"), QStringLiteral("70"));

    parser.addOption(messageOpt);
    parser.addOption(countdownOpt);
    parser.addOption(snoozeOpt);
    parser.addOption(actionOpt);
    parser.addOption(soundEnabledOpt);
    parser.addOption(soundFileOpt);
    parser.addOption(volumeOpt);

    parser.process(app);

    const QString message = parser.value(messageOpt);
    const int countdownSeconds = parser.value(countdownOpt).toInt();
    const int snoozeMinutes = parser.value(snoozeOpt).toInt();
    const QString actionLabel = parser.value(actionOpt);
    const bool soundEnabled = parser.isSet(soundEnabledOpt);
    const QString soundFile = parser.value(soundFileOpt);
    const int soundVolume = parser.value(volumeOpt).toInt();

    WarningBanner banner(QObject::tr("%1\nAction: %2").arg(message, actionLabel), countdownSeconds);
    banner.setSoundOptions(soundEnabled, soundFile, soundVolume);
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
