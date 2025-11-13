#include "AppConfig.h"

#include <QList>

AppConfig::AppConfig() {
    if (!weekly.isEmpty()) {
        return;
    }

    const QList<Qt::DayOfWeek> days {
        Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday,
        Qt::Friday, Qt::Saturday, Qt::Sunday
    };

    weekly.reserve(days.size());
    for (auto day : days) {
        WeeklyEntry entry;
        entry.day = day;
        entry.enabled = false;
        entry.shutdownTime = QTime(23, 0);
        entry.wakeTime = QTime(7, 30);
        weekly.push_back(entry);
    }
}
