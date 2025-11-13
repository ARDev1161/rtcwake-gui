#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <QDateTime>

/** Enumeration describing which power transition should be executed. */
enum class PowerAction {
    None = 0,
    SuspendToIdle,
    SuspendToRam,
    Hibernate,
    PowerOff
};

/**
 * @brief Thin wrapper around the rtcwake utility.
 */
class RtcWakeController : public QObject {
    Q_OBJECT

public:
    struct CommandResult {
        bool success {false};
        QString stdOut;
        QString stdErr;
        QString commandLine;
    };

    explicit RtcWakeController(QObject *parent = nullptr);

    CommandResult scheduleWake(const QDateTime &targetUtc, PowerAction action) const;
    CommandResult executePowerAction(PowerAction action) const;

    static QString actionLabel(PowerAction action);
    static QString rtcwakeMode(PowerAction action);

private:
    CommandResult runProcess(const QStringList &arguments) const;
};
