#pragma once

#include "RtcWakeController.h"

#include <QDate>
#include <QTime>
#include <QString>
#include <QVector>

/** User-configurable warning dialog preferences. */
struct WarningPreferences {
    bool enabled {true};
    QString message {QStringLiteral("System will suspend soon. Save your work.")};
    int countdownSeconds {30};
    int snoozeMinutes {5};
};

/** Entry representing a weekly schedule row. */
struct WeeklyEntry {
    Qt::DayOfWeek day {Qt::Monday};
    bool enabled {false};
    QTime time {QTime(7, 30)};
};

/** Aggregate structure storing everything we persist between runs. */
struct AppConfig {
    AppConfig();

    QDate singleDate {QDate::currentDate()};
    QTime singleTime {QTime::currentTime()};
    int actionId {static_cast<int>(PowerAction::SuspendToRam)};
    WarningPreferences warning;
    QVector<WeeklyEntry> weekly;
};
