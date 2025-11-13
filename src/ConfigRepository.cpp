#include "ConfigRepository.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>
#include <QTime>

namespace {
QString formatTime(const QTime &time) {
    return time.toString(QStringLiteral("HH:mm"));
}
}

AppConfig ConfigRepository::load() const {
    AppConfig config;
    QFile file(configPath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        return config;
    }

    return parse(data);
}

bool ConfigRepository::save(const AppConfig &config) const {
    const QString path = configPath();
    QFile file(path);
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists() && !QDir().mkpath(dir.absolutePath())) {
        qWarning() << "Failed to create config dir" << dir.absolutePath();
        return false;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Failed to open config file" << path << file.errorString();
        return false;
    }

    const QByteArray payload = serialize(config);
    const auto written = file.write(payload);
    file.write("\n");
    return written == payload.size();
}

QString ConfigRepository::configPath() const {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QStringLiteral("/.config");
    }
    return base + QStringLiteral("/rtcwake-gui/config.json");
}

AppConfig ConfigRepository::parse(const QByteArray &json) const {
    AppConfig config;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return config;
    }

    const QJsonObject root = doc.object();
    const auto singleDate = QDate::fromString(root.value(QStringLiteral("singleDate")).toString(), Qt::ISODate);
    const auto singleTime = QTime::fromString(root.value(QStringLiteral("singleTime")).toString(), Qt::ISODate);
    if (singleDate.isValid()) {
        config.singleDate = singleDate;
    }
    if (singleTime.isValid()) {
        config.singleTime = singleTime;
    }

    config.actionId = root.value(QStringLiteral("actionId")).toInt(config.actionId);

    const auto warningObj = root.value(QStringLiteral("warning")).toObject();
    if (!warningObj.isEmpty()) {
        config.warning.enabled = warningObj.value(QStringLiteral("enabled")).toBool(config.warning.enabled);
        config.warning.message = warningObj.value(QStringLiteral("message")).toString(config.warning.message);
        config.warning.countdownSeconds = warningObj.value(QStringLiteral("countdownSeconds")).toInt(config.warning.countdownSeconds);
        config.warning.snoozeMinutes = warningObj.value(QStringLiteral("snoozeMinutes")).toInt(config.warning.snoozeMinutes);
    }

    QHash<int, WeeklyEntry> overrides;
    const auto weeklyArray = root.value(QStringLiteral("weekly")).toArray();
    for (const auto &value : weeklyArray) {
        const QJsonObject obj = value.toObject();
        const int day = obj.value(QStringLiteral("day")).toInt(-1);
        if (day < static_cast<int>(Qt::Monday) || day > static_cast<int>(Qt::Sunday)) {
            continue;
        }
        WeeklyEntry entry;
        entry.day = static_cast<Qt::DayOfWeek>(day);
        entry.enabled = obj.value(QStringLiteral("enabled")).toBool(false);
        const QString timeStr = obj.value(QStringLiteral("time")).toString();
        const auto parsedTime = QTime::fromString(timeStr, QStringLiteral("HH:mm"));
        if (parsedTime.isValid()) {
            entry.time = parsedTime;
        }
        overrides.insert(day, entry);
    }

    for (auto &entry : config.weekly) {
        const int day = static_cast<int>(entry.day);
        if (overrides.contains(day)) {
            entry = overrides.value(day);
        }
    }

    return config;
}

QByteArray ConfigRepository::serialize(const AppConfig &config) const {
    QJsonObject root;
    root.insert(QStringLiteral("singleDate"), config.singleDate.toString(Qt::ISODate));
    root.insert(QStringLiteral("singleTime"), config.singleTime.toString(Qt::ISODate));
    root.insert(QStringLiteral("actionId"), config.actionId);

    QJsonObject warningObj;
    warningObj.insert(QStringLiteral("enabled"), config.warning.enabled);
    warningObj.insert(QStringLiteral("message"), config.warning.message);
    warningObj.insert(QStringLiteral("countdownSeconds"), config.warning.countdownSeconds);
    warningObj.insert(QStringLiteral("snoozeMinutes"), config.warning.snoozeMinutes);
    root.insert(QStringLiteral("warning"), warningObj);

    QJsonArray weeklyArray;
    for (const auto &entry : config.weekly) {
        QJsonObject obj;
        obj.insert(QStringLiteral("day"), static_cast<int>(entry.day));
        obj.insert(QStringLiteral("enabled"), entry.enabled);
        obj.insert(QStringLiteral("time"), formatTime(entry.time));
        weeklyArray.append(obj);
    }
    root.insert(QStringLiteral("weekly"), weeklyArray);

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}
