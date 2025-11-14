#include "RtcWakeDaemon.h"

#include "SchedulePlanner.h"
#include "SummaryWriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QProcess>
#include <QLocale>
#include <limits>
#include <algorithm>

namespace {
constexpr int kPeriodicIntervalMs = 5 * 60 * 1000; // 5 minutes

QString formatDateTime(const QDateTime &dt) {
    return QLocale().toString(dt, QLocale::LongFormat);
}
}

RtcWakeDaemon::RtcWakeDaemon(Options options, QObject *parent)
    : QObject(parent),
      m_repo(options.configPath),
      m_options(std::move(options)) {
    connect(&m_periodic, &QTimer::timeout, this, &RtcWakeDaemon::handlePeriodic);
    connect(&m_eventTimer, &QTimer::timeout, this, &RtcWakeDaemon::handleEventTimeout);
    m_periodic.setInterval(kPeriodicIntervalMs);
    m_periodic.setSingleShot(false);
    m_eventTimer.setSingleShot(true);
}

void RtcWakeDaemon::start() {
    log(tr("Daemon starting (PID %1)").arg(QCoreApplication::applicationPid()));
    watchConfig();
    reloadConfig();
    m_periodic.start();
}

void RtcWakeDaemon::watchConfig() {
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!m_options.configPath.isEmpty()) {
        QFileInfo info(m_options.configPath);
        if (info.exists()) {
            m_watcher.addPath(info.absoluteFilePath());
        }
        if (!info.path().isEmpty()) {
            m_watcher.addPath(info.path());
        }
    }
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &RtcWakeDaemon::handleConfigChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &RtcWakeDaemon::handleConfigChanged);
}

void RtcWakeDaemon::handleConfigChanged() {
    QTimer::singleShot(500, this, [this]() { reloadConfig(); });
}

void RtcWakeDaemon::handlePeriodic() {
    planNext(tr("Periodic refresh"));
}

void RtcWakeDaemon::reloadConfig() {
    m_config = m_repo.load();
    planNext(tr("Config reloaded"));
}

void RtcWakeDaemon::planNext(const QString &reason) {
    SchedulePlanner::Event next;
    const QDateTime now = QDateTime::currentDateTime();
    if (!SchedulePlanner::nextEvent(m_config, now, next)) {
        cancelEventTimer();
        log(tr("No upcoming events. %1").arg(reason));
        return;
    }

    m_nextShutdown = next.shutdown;
    m_nextWake = next.wake;
    m_nextAction = next.action;

    programAlarm(next.wake, next.action);
    scheduleEventTimer(next.shutdown, next.action);
    SummaryWriter::write(m_options.targetHome, next.wake, next.action);

    log(tr("Next shutdown at %1, wake at %2 (%3)")
            .arg(formatDateTime(next.shutdown), formatDateTime(next.wake), RtcWakeController::actionLabel(next.action)));
}

void RtcWakeDaemon::scheduleEventTimer(const QDateTime &shutdown, PowerAction action) {
    m_eventTimer.stop();
    m_nextShutdown = shutdown;
    m_nextAction = action;

    const qint64 msecs = QDateTime::currentDateTime().msecsTo(shutdown);
    if (msecs <= 0) {
        QTimer::singleShot(0, this, &RtcWakeDaemon::handleEventTimeout);
        return;
    }
    m_eventTimer.start(static_cast<int>(qMin(msecs, static_cast<qint64>(std::numeric_limits<int>::max()))));
}

void RtcWakeDaemon::cancelEventTimer() {
    m_eventTimer.stop();
    m_nextShutdown = QDateTime();
}

void RtcWakeDaemon::handleEventTimeout() {
    if (!m_nextShutdown.isValid()) {
        return;
    }

    const auto outcome = invokeWarning(m_nextShutdown, m_nextAction);
    if (outcome == WarningOutcome::Snooze) {
        const int snoozeMs = m_config.warning.snoozeMinutes * 60 * 1000;
        m_nextShutdown = QDateTime::currentDateTime().addMSecs(snoozeMs);
        scheduleEventTimer(m_nextShutdown, m_nextAction);
        log(tr("Power action snoozed for %1 minutes").arg(m_config.warning.snoozeMinutes));
        return;
    }

    if (outcome == WarningOutcome::Cancel) {
        log(tr("Power action canceled by user"));
        planNext(tr("User canceled"));
        return;
    }

    if (m_nextAction != PowerAction::None) {
        auto result = m_controller.executePowerAction(m_nextAction);
        if (!result.success) {
            log(tr("Failed to run power action: %1").arg(result.stdErr));
        } else {
            log(tr("Executed %1").arg(RtcWakeController::actionLabel(m_nextAction)));
        }
    }

    QTimer::singleShot(0, this, [this]() { planNext(tr("Action completed")); });
}

void RtcWakeDaemon::programAlarm(const QDateTime &wake, PowerAction action) {
    Q_UNUSED(action);
    auto result = m_controller.programAlarm(wake.toUTC());
    if (!result.success) {
        log(tr("Failed to program rtcwake: %1").arg(result.stdErr));
    }
}

RtcWakeDaemon::WarningOutcome RtcWakeDaemon::invokeWarning(const QDateTime &shutdown, PowerAction action) {
    if (!m_config.warning.enabled || m_options.warningApp.isEmpty()) {
        return WarningOutcome::Apply;
    }

    QString program = QStringLiteral("runuser");
    QStringList args;
    args << QStringLiteral("-u") << m_options.targetUser
         << QStringLiteral("--") << QStringLiteral("env");

    const auto env = buildUserEnvironment();
    if (!env.value(QStringLiteral("DISPLAY")).isEmpty()) {
        args << QStringLiteral("DISPLAY=") + env.value(QStringLiteral("DISPLAY"));
    }
    if (!env.value(QStringLiteral("XDG_RUNTIME_DIR")).isEmpty()) {
        args << QStringLiteral("XDG_RUNTIME_DIR=") + env.value(QStringLiteral("XDG_RUNTIME_DIR"));
    }
    if (!env.value(QStringLiteral("DBUS_SESSION_BUS_ADDRESS")).isEmpty()) {
        args << QStringLiteral("DBUS_SESSION_BUS_ADDRESS=") + env.value(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"));
    }

    args << m_options.warningApp;
    args << QStringLiteral("--message") << m_config.warning.message;
    args << QStringLiteral("--countdown") << QString::number(m_config.warning.countdownSeconds);
    args << QStringLiteral("--snooze") << QString::number(m_config.warning.snoozeMinutes);
    const QString theme = m_config.warning.theme.isEmpty() ? QStringLiteral("crimson") : m_config.warning.theme;
    args << QStringLiteral("--theme") << theme;
    if (m_config.warning.fullscreen) {
        args << QStringLiteral("--fullscreen");
    }
    const int width = std::clamp(m_config.warning.width, 320, 3840);
    const int height = std::clamp(m_config.warning.height, 200, 2160);
    args << QStringLiteral("--width") << QString::number(width);
    args << QStringLiteral("--height") << QString::number(height);
    if (m_config.warning.soundEnabled) {
        args << QStringLiteral("--sound-enabled");
        if (!m_config.warning.soundFile.isEmpty()) {
            args << QStringLiteral("--sound-file") << m_config.warning.soundFile;
        }
        const int volume = std::clamp(m_config.warning.soundVolume, 0, 100);
        args << QStringLiteral("--volume") << QString::number(volume);
    }
    args << QStringLiteral("--action") << RtcWakeController::actionLabel(action);

    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted() || !process.waitForFinished(-1)) {
        log(tr("Failed to start warning dialog"));
        return WarningOutcome::Apply;
    }

    const int exitCode = process.exitCode();
    if (exitCode == 0) {
        return WarningOutcome::Apply;
    }
    if (exitCode == 1) {
        return WarningOutcome::Snooze;
    }
    return WarningOutcome::Cancel;
}

QProcessEnvironment RtcWakeDaemon::buildUserEnvironment() const {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_config.session.display.isEmpty()) {
        env.insert(QStringLiteral("DISPLAY"), m_config.session.display);
    }
    if (!m_config.session.xdgRuntimeDir.isEmpty()) {
        env.insert(QStringLiteral("XDG_RUNTIME_DIR"), m_config.session.xdgRuntimeDir);
    }
    if (!m_config.session.dbusAddress.isEmpty()) {
        env.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"), m_config.session.dbusAddress);
    }
    if (!m_config.session.xauthority.isEmpty()) {
        env.insert(QStringLiteral("XAUTHORITY"), m_config.session.xauthority);
    }
    if (!m_config.session.waylandDisplay.isEmpty()) {
        env.insert(QStringLiteral("WAYLAND_DISPLAY"), m_config.session.waylandDisplay);
    }
    return env;
}

void RtcWakeDaemon::log(const QString &message) const {
    qInfo().noquote() << message;
}
