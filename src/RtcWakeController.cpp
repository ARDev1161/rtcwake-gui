#include "RtcWakeController.h"

#include <QProcess>

RtcWakeController::RtcWakeController(QObject *parent)
    : QObject(parent) {}

RtcWakeController::CommandResult RtcWakeController::scheduleWake(const QDateTime &targetUtc, PowerAction action) const {
    const QString epoch = QString::number(targetUtc.toSecsSinceEpoch());
    const QString mode = rtcwakeMode(action);

    QStringList args {QStringLiteral("rtcwake"), QStringLiteral("-m"), mode, QStringLiteral("-t"), epoch};
    return runProcess(args);
}

RtcWakeController::CommandResult RtcWakeController::programAlarm(const QDateTime &targetUtc) const {
    const QString epoch = QString::number(targetUtc.toSecsSinceEpoch());
    QStringList args {QStringLiteral("rtcwake"), QStringLiteral("-m"), QStringLiteral("no"), QStringLiteral("-t"), epoch};
    return runProcess(args);
}

QString RtcWakeController::actionLabel(PowerAction action) {
    switch (action) {
    case PowerAction::SuspendToIdle:
        return QObject::tr("Suspend to idle");
    case PowerAction::SuspendToRam:
        return QObject::tr("Suspend to RAM");
    case PowerAction::Hibernate:
        return QObject::tr("Hibernate");
    case PowerAction::PowerOff:
        return QObject::tr("Power off");
    case PowerAction::None:
    default:
        return QObject::tr("Do nothing");
    }
}

QString RtcWakeController::rtcwakeMode(PowerAction action) {
    switch (action) {
    case PowerAction::SuspendToIdle:
        return QStringLiteral("freeze");
    case PowerAction::SuspendToRam:
        return QStringLiteral("mem");
    case PowerAction::Hibernate:
        return QStringLiteral("disk");
    case PowerAction::PowerOff:
        return QStringLiteral("off");
    case PowerAction::None:
    default:
        return QStringLiteral("no");
    }
}

RtcWakeController::CommandResult RtcWakeController::runProcess(const QStringList &arguments) const {
    CommandResult result;

    if (arguments.isEmpty()) {
        result.stdErr = QObject::tr("Internal error: empty command");
        return result;
    }

    QProcess process;
    QString program = arguments.first();
    QStringList args = arguments;
    args.removeFirst();

    result.commandLine = arguments.join(' ');

    process.start(program, args);
    bool started = process.waitForStarted();
    if (!started) {
        result.stdErr = QObject::tr("Failed to start %1").arg(program);
        return result;
    }

    process.waitForFinished(-1);
    result.stdOut = QString::fromLocal8Bit(process.readAllStandardOutput());
    result.stdErr = QString::fromLocal8Bit(process.readAllStandardError());
    result.success = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    return result;
}
