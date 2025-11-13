#include "SchedulePlanner.h"

#include <QTimeZone>

namespace SchedulePlanner {

namespace {
void considerCandidate(const QDateTime &shutdown, const QDateTime &wake, PowerAction action,
                       const QDateTime &now, bool &hasCandidate, Event &best) {
    if (!shutdown.isValid() || !wake.isValid()) {
        return;
    }
    if (shutdown <= now) {
        return;
    }
    if (!hasCandidate || shutdown < best.shutdown) {
        best.shutdown = shutdown;
        best.wake = wake;
        best.action = action;
        hasCandidate = true;
    }
}

QDateTime buildDateTime(const QDate &date, const QTime &time, const QTimeZone &zone) {
    return QDateTime(date, time, zone);
}
}

bool nextEvent(const AppConfig &config, const QDateTime &now, Event &event) {
    bool hasCandidate = false;
    const QTimeZone zone = now.timeZone();
    const PowerAction action = static_cast<PowerAction>(config.actionId);

    const QDateTime singleShutdown = buildDateTime(config.singleShutdownDate, config.singleShutdownTime, zone);
    const QDateTime singleWake = buildDateTime(config.singleWakeDate, config.singleWakeTime, zone);
    if (singleShutdown.isValid() && singleWake.isValid() && singleShutdown < singleWake) {
        considerCandidate(singleShutdown, singleWake, action, now, hasCandidate, event);
    }

    for (const auto &entry : config.weekly) {
        if (!entry.enabled) {
            continue;
        }

        QDate baseDate = now.date();
        int offset = static_cast<int>(entry.day) - baseDate.dayOfWeek();
        if (offset < 0) {
            offset += 7;
        }
        QDate shutdownDate = baseDate.addDays(offset);
        QDateTime shutdown = buildDateTime(shutdownDate, entry.shutdownTime, zone);
        if (shutdown <= now) {
            shutdown = shutdown.addDays(7);
        }

        QDateTime wake = buildDateTime(shutdown.date(), entry.wakeTime, zone);
        if (wake <= shutdown) {
            wake = wake.addDays(1);
        }

        considerCandidate(shutdown, wake, action, now, hasCandidate, event);
    }

    return hasCandidate;
}

} // namespace SchedulePlanner
