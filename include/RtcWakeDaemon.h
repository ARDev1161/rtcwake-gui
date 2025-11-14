#pragma once

#include "AppConfig.h"
#include "ConfigRepository.h"
#include "RtcWakeController.h"

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QObject>
#include <QProcess>
#include <QTimer>

class RtcWakeDaemon : public QObject {
    Q_OBJECT

public:
    struct Options {
        QString configPath;
        QString targetUser;
        QString targetHome;
        QString warningApp;
    };

    explicit RtcWakeDaemon(Options options, QObject *parent = nullptr);

    void start();

private slots:
    void handleConfigChanged();
    void handlePeriodic();
    void handleEventTimeout();

private:
    void watchConfig();
    void reloadConfig();
    void planNext(const QString &reason = QString());
    void scheduleEventTimer(const QDateTime &shutdown, PowerAction action);
    void cancelEventTimer();
    void programAlarm(const QDateTime &wake, PowerAction action);
    void log(const QString &message) const;
    QString resolveLogPath() const;
    void appendRtcwakeLog(const QString &actionLabel, const QString &wakeLabel,
                          const RtcWakeController::CommandResult &result) const;

    enum class WarningOutcome {
        Apply,
        Snooze,
        Cancel
    };

    WarningOutcome invokeWarning(const QDateTime &shutdown, PowerAction action);
    QProcessEnvironment buildUserEnvironment() const;

    ConfigRepository m_repo;
    AppConfig m_config;
    Options m_options;
    QFileSystemWatcher m_watcher;
    QTimer m_periodic;
    QTimer m_eventTimer;
    QDateTime m_nextShutdown;
    QDateTime m_nextWake;
    PowerAction m_nextAction {PowerAction::None};
    RtcWakeController m_controller;
    QString m_rtcwakeLogPath;
};
