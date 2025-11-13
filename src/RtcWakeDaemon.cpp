#include "RtcWakeDaemon.h"

#include "SummaryWriter.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QLocale>
#include <QTimeZone>

namespace {
QString formatRelative(const QDateTime &target) {
    const QDateTime now = QDateTime::currentDateTime();
    return QStringLiteral("%1 (%2 min)")
        .arg(QLocale().toString(target, QLocale::LongFormat))
        .arg(now.msecsTo(target) / 60000);
}
}

RtcWakeDaemon::RtcWakeDaemon(QObject *parent)
    : QObject(parent) {
    connect(&m_periodic, &QTimer::timeout, this, &RtcWakeDaemon::handlePeriodic);
    m_periodic.setInterval(5 * 60 * 1000); // every 5 minutes
    m_periodic.setSingleShot(false);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &RtcWakeDaemon::handleConfigChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &RtcWakeDaemon::handleConfigChanged);
}

void RtcWakeDaemon::start() {
    watchConfig();
    reloadConfig();
    m_periodic.start();
    log(QStringLiteral("rtcwake-daemon started (PID %1)").arg(QCoreApplication::applicationPid()));
}

void RtcWakeDaemon::handleConfigChanged() {
    // Delay to debounce rapid save events
    QTimer::singleShot(500, this, [this]() {
        reloadConfig();
    });
}

void RtcWakeDaemon::handlePeriodic() {
    planNext();
}

void RtcWakeDaemon::watchConfig() {
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
    }

    const QString path = m_repo.configPath();
    QFileInfo info(path);
    if (!info.absolutePath().isEmpty()) {
        m_watcher.addPath(info.absolutePath());
    }
    if (QFile::exists(path)) {
        m_watcher.addPath(path);
    }
}

void RtcWakeDaemon::reloadConfig() {
    m_config = m_repo.load();
    planNext();
}

bool RtcWakeDaemon::computeNext(QDateTime &targetLocal, PowerAction &action) const {
    const QDateTime now = QDateTime::currentDateTime();
    bool hasCandidate = false;
    QDateTime best;

    QDateTime single(m_config.singleDate, m_config.singleTime, now.timeZone());
    if (single.isValid() && single > now) {
        best = single;
        hasCandidate = true;
    }

    for (const auto &entry : m_config.weekly) {
        if (!entry.enabled) {
            continue;
        }
        int offset = static_cast<int>(entry.day) - now.date().dayOfWeek();
        if (offset < 0) {
            offset += 7;
        }
        QDate candidateDate = now.date().addDays(offset);
        QDateTime candidate(candidateDate, entry.time, now.timeZone());
        if (candidate <= now) {
            candidate = candidate.addDays(7);
        }
        if (!hasCandidate || candidate < best) {
            best = candidate;
            hasCandidate = true;
        }
    }

    if (!hasCandidate) {
        return false;
    }

    targetLocal = best;
    action = static_cast<PowerAction>(m_config.actionId);
    return true;
}

void RtcWakeDaemon::planNext() {
    QDateTime targetLocal;
    PowerAction action = PowerAction::None;
    if (!computeNext(targetLocal, action)) {
        log(QStringLiteral("No upcoming events found in config."));
        return;
    }

    if (m_lastArmed.isValid() && m_lastArmed == targetLocal) {
        return; // already armed
    }

    auto result = m_controller.programAlarm(targetLocal.toUTC());
    if (!result.success) {
        log(QStringLiteral("Failed to program RTC: %1").arg(result.stdErr));
        return;
    }

    SummaryWriter::write(targetLocal, action);
    m_lastArmed = targetLocal;
    log(QStringLiteral("Armed wake for %1").arg(formatRelative(targetLocal)));
}

void RtcWakeDaemon::log(const QString &message) const {
    qInfo().noquote() << message;
}
