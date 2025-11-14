#include "RtcWakeDaemon.h"

#include "SchedulePlanner.h"
#include "SummaryWriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QProcess>
#include <QLocale>
#include <QTextStream>
#include <limits>
#include <algorithm>

namespace {
constexpr int kPeriodicIntervalMs = 5 * 60 * 1000; // 5 minutes

QString formatDateTime(const QDateTime &dt) {
    return QLocale().toString(dt, QLocale::LongFormat);
}

QString sanitizeSingleLine(QString text) {
    text.replace(QLatin1Char('\r'), QLatin1Char(' '));
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return text.trimmed();
}
}

RtcWakeDaemon::RtcWakeDaemon(Options options, QObject *parent)
    : QObject(parent),
      m_repo(options.configPath),
      m_options(std::move(options)),
      m_rtcwakeLogPath(resolveLogPath()) {
    connect(&m_periodic, &QTimer::timeout, this, &RtcWakeDaemon::handlePeriodic);
    connect(&m_eventTimer, &QTimer::timeout, this, &RtcWakeDaemon::handleEventTimeout);
    m_periodic.setInterval(kPeriodicIntervalMs);
    m_periodic.setSingleShot(false);
    m_eventTimer.setSingleShot(true);
}

void RtcWakeDaemon::start() {
    log(tr("Daemon starting (PID %1)").arg(QCoreApplication::applicationPid()));
    appendPersistentLog(QStringLiteral("daemon_start"),
                        {{QStringLiteral("pid"), QString::number(QCoreApplication::applicationPid())}});
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
    appendPersistentLog(QStringLiteral("config_watch"), {{QStringLiteral("event"), QStringLiteral("changed")}});
}

void RtcWakeDaemon::handlePeriodic() {
    planNext(tr("Periodic refresh"));
}

void RtcWakeDaemon::reloadConfig() {
    m_config = m_repo.load();
    planNext(tr("Config reloaded"));
    appendPersistentLog(QStringLiteral("config_reload"),
                        {{QStringLiteral("path"), m_options.configPath.isEmpty() ? tr("<default>") : m_options.configPath}});
}

void RtcWakeDaemon::planNext(const QString &reason) {
    SchedulePlanner::Event next;
    const QDateTime now = QDateTime::currentDateTime();
    if (!SchedulePlanner::nextEvent(m_config, now, next)) {
        cancelEventTimer();
        log(tr("No upcoming events. %1").arg(reason));
        appendPersistentLog(QStringLiteral("schedule"),
                            {{QStringLiteral("status"), QStringLiteral("empty")},
                             {QStringLiteral("reason"), reason.isEmpty() ? tr("<unspecified>") : reason}});
        return;
    }

    m_nextShutdown = next.shutdown;
    m_nextWake = next.wake;
    m_nextAction = next.action;

    if (next.action == PowerAction::None) {
        programAlarm(next.wake, next.action);
    }
    scheduleEventTimer(next.shutdown, next.action);
    SummaryWriter::write(m_options.targetHome, next.wake, next.action);

    const QString shutdownLabel = formatDateTime(next.shutdown);
    const QString wakeLabel = formatDateTime(next.wake);
    const QString actionLabel = RtcWakeController::actionLabel(next.action);
    log(tr("Next shutdown at %1, wake at %2 (%3)")
            .arg(shutdownLabel, wakeLabel, actionLabel));
    appendPersistentLog(QStringLiteral("schedule"),
                        {{QStringLiteral("status"), QStringLiteral("planned")},
                         {QStringLiteral("reason"), reason.isEmpty() ? tr("<unspecified>") : reason},
                         {QStringLiteral("shutdown"), shutdownLabel},
                         {QStringLiteral("wake"), wakeLabel},
                         {QStringLiteral("action"), actionLabel}});
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
        appendPersistentLog(QStringLiteral("warning"),
                            {{QStringLiteral("outcome"), QStringLiteral("snooze")},
                             {QStringLiteral("minutes"), QString::number(m_config.warning.snoozeMinutes)}});
        return;
    }

    if (outcome == WarningOutcome::Cancel) {
        log(tr("Power action canceled by user"));
        appendPersistentLog(QStringLiteral("warning"),
                            {{QStringLiteral("outcome"), QStringLiteral("cancel")}});
        planNext(tr("User canceled"));
        return;
    }

    if (m_nextAction == PowerAction::None) {
        log(tr("No power transition requested; nothing to execute"));
        appendPersistentLog(QStringLiteral("action"),
                            {{QStringLiteral("status"), QStringLiteral("skipped")},
                             {QStringLiteral("reason"), QStringLiteral("no_action")}});
    } else if (!m_nextWake.isValid()) {
        log(tr("Cannot arm rtcwake: next wake time is invalid"));
        appendPersistentLog(QStringLiteral("action"),
                            {{QStringLiteral("status"), QStringLiteral("skipped")},
                             {QStringLiteral("reason"), QStringLiteral("invalid_wake")}});
    } else {
        const QString actionLabel = RtcWakeController::actionLabel(m_nextAction);
        const QString wakeLabel = formatDateTime(m_nextWake);
        auto result = m_controller.scheduleWake(m_nextWake.toUTC(), m_nextAction);
        if (!result.success) {
            log(tr("Failed to arm rtcwake for %1 via %2: %3")
                    .arg(actionLabel,
                         result.commandLine.isEmpty() ? tr("<unknown command>") : result.commandLine,
                         result.stdErr.isEmpty() ? tr("<no stderr>") : result.stdErr));
        } else {
            log(tr("Invoked rtcwake for %1 at %2 via: %3")
                    .arg(actionLabel,
                         wakeLabel,
                         result.commandLine.isEmpty() ? tr("<unknown command>") : result.commandLine));
        }
        appendPersistentLog(QStringLiteral("rtcwake"),
                            {{QStringLiteral("action"), actionLabel},
                             {QStringLiteral("wake"), wakeLabel},
                             {QStringLiteral("command"), result.commandLine.isEmpty() ? tr("<unknown>") : result.commandLine},
                             {QStringLiteral("exit"), QString::number(result.exitCode)},
                             {QStringLiteral("success"), result.success ? QStringLiteral("true") : QStringLiteral("false")},
                             {QStringLiteral("stderr"), result.stdErr.isEmpty() ? tr("<empty>") : result.stdErr}});
    }

    QTimer::singleShot(0, this, [this]() { planNext(tr("Action completed")); });
}

void RtcWakeDaemon::programAlarm(const QDateTime &wake, PowerAction action) {
    Q_UNUSED(action);
    const QString wakeLabel = wake.isValid() ? formatDateTime(wake) : tr("<invalid wake time>");
    auto result = m_controller.programAlarm(wake.toUTC());
    if (result.success) {
        log(tr("Programmed rtcwake for %1 via: %2")
                .arg(wakeLabel,
                     result.commandLine.isEmpty() ? tr("<unknown command>") : result.commandLine));
    } else {
        log(tr("Failed to program rtcwake for %1 via %2: %3")
                .arg(wakeLabel,
                     result.commandLine.isEmpty() ? tr("<unknown command>") : result.commandLine,
                     result.stdErr.isEmpty() ? tr("<no stderr>") : result.stdErr));
    }
    appendPersistentLog(QStringLiteral("rtcwake"),
                        {{QStringLiteral("action"), RtcWakeController::actionLabel(PowerAction::None)},
                         {QStringLiteral("wake"), wakeLabel},
                         {QStringLiteral("command"), result.commandLine.isEmpty() ? tr("<unknown>") : result.commandLine},
                         {QStringLiteral("exit"), QString::number(result.exitCode)},
                         {QStringLiteral("success"), result.success ? QStringLiteral("true") : QStringLiteral("false")},
                         {QStringLiteral("stderr"), result.stdErr.isEmpty() ? tr("<empty>") : result.stdErr}});
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
    if (!env.value(QStringLiteral("XAUTHORITY")).isEmpty()) {
        args << QStringLiteral("XAUTHORITY=") + env.value(QStringLiteral("XAUTHORITY"));
    }
    if (!env.value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty()) {
        args << QStringLiteral("WAYLAND_DISPLAY=") + env.value(QStringLiteral("WAYLAND_DISPLAY"));
    }
    if (!m_options.targetHome.isEmpty()) {
        args << QStringLiteral("HOME=") + m_options.targetHome;
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

    const QString stdoutText = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    const QString stderrText = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
    const int exitCode = process.exitCode();
    if (exitCode != 0) {
        log(tr("Warning dialog exited with %1 (stdout: %2, stderr: %3)")
                .arg(exitCode)
                .arg(stdoutText.isEmpty() ? QStringLiteral("<empty>") : stdoutText)
                .arg(stderrText.isEmpty() ? QStringLiteral("<empty>") : stderrText));
    }
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

QString RtcWakeDaemon::resolveLogPath() const {
    QString base = m_options.targetHome;
    if (base.isEmpty()) {
        base = QDir::homePath();
    }
    if (base.isEmpty()) {
        return QString();
    }
    QDir dir(base);
    return dir.filePath(QStringLiteral(".local/share/rtcwake-gui/log.txt"));
}

void RtcWakeDaemon::appendPersistentLog(const QString &category, const QList<QPair<QString, QString>> &fields) const {
    if (m_rtcwakeLogPath.isEmpty()) {
        return;
    }

    QFileInfo info(m_rtcwakeLogPath);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qWarning().noquote() << "Failed to create log directory" << dir.absolutePath();
        return;
    }

    QFile file(info.filePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning().noquote() << "Failed to open log file" << info.filePath();
        return;
    }

    QTextStream stream(&file);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    stream << "[" << stamp << "] category=\"" << sanitizeSingleLine(category) << "\"";
    for (const auto &pair : fields) {
        stream << " " << pair.first << "=\""
               << sanitizeSingleLine(pair.second) << "\"";
    }
    stream << "\n";
}
