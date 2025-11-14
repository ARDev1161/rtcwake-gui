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
    /** Result bundle returned from every process invocation. */
    struct CommandResult {
        bool success {false};
        QString stdOut;
        QString stdErr;
        QString commandLine;
    };

    explicit RtcWakeController(QObject *parent = nullptr);

    /**
     * @brief Program the RTC using rtcwake.
     * @param targetUtc Absolute wake time in UTC.
     * @param action Power transition to execute immediately.
     */
    CommandResult scheduleWake(const QDateTime &targetUtc, PowerAction action) const;

    /**
     * @brief Only set the RTC alarm without leaving the current power state.
     */
    CommandResult programAlarm(const QDateTime &targetUtc) const;

    static QString actionLabel(PowerAction action);
    static QString rtcwakeMode(PowerAction action);

private:
    CommandResult runProcess(const QStringList &arguments) const;
};
