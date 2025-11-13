#include "SchedulePlanner.h"

#include <QTimeZone>

namespace SchedulePlanner {

bool nextEvent(const AppConfig &config, const QDateTime &now, QDateTime &targetLocal, PowerAction &action) {
    bool hasCandidate = false;
    QDateTime best;

    QDateTime single(config.singleDate, config.singleTime, now.timeZone());
    if (single.isValid() && single > now) {
        best = single;
        hasCandidate = true;
    }

    for (const auto &entry : config.weekly) {
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
    action = static_cast<PowerAction>(config.actionId);
    return true;
}

} // namespace SchedulePlanner
