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
    QTime shutdownTime {QTime(23, 0)};
    QTime wakeTime {QTime(7, 30)};
};

/** Details about the user's graphical session so the daemon can show banners. */
struct SessionInfo {
    QString user;
    QString display;
    QString xdgRuntimeDir;
    QString dbusAddress;
};

/** Aggregate structure storing everything we persist between runs. */
struct AppConfig {
    AppConfig();

    QDate singleShutdownDate {QDate::currentDate()};
    QTime singleShutdownTime {QTime::currentTime()};
    QDate singleWakeDate {QDate::currentDate()};
    QTime singleWakeTime {QTime::currentTime()};
    int actionId {static_cast<int>(PowerAction::SuspendToRam)};
    WarningPreferences warning;
    QVector<WeeklyEntry> weekly;
    SessionInfo session;
};
