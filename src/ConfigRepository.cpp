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

ConfigRepository::ConfigRepository() = default;

ConfigRepository::ConfigRepository(QString explicitPath)
    : m_explicitPath(std::move(explicitPath)) {}

QString ConfigRepository::resolvedPath() const {
    if (!m_explicitPath.isEmpty()) {
        return m_explicitPath;
    }
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + QStringLiteral("/.config");
    }
    return base + QStringLiteral("/rtcwake-gui/config.json");
}

AppConfig ConfigRepository::load() const {
    AppConfig config;
    QFile file(resolvedPath());
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
    const QString path = resolvedPath();
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
    return resolvedPath();
}

AppConfig ConfigRepository::parse(const QByteArray &json) const {
    AppConfig config;
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return config;
    }

    const QJsonObject root = doc.object();
    const auto singleShutdownDate = QDate::fromString(root.value(QStringLiteral("singleShutdownDate")).toString(), Qt::ISODate);
    const auto singleShutdownTime = QTime::fromString(root.value(QStringLiteral("singleShutdownTime")).toString(), Qt::ISODate);
    const auto singleWakeDate = QDate::fromString(root.value(QStringLiteral("singleDate")).toString(), Qt::ISODate);
    const auto singleWakeTime = QTime::fromString(root.value(QStringLiteral("singleTime")).toString(), Qt::ISODate);
    if (singleShutdownDate.isValid()) {
        config.singleShutdownDate = singleShutdownDate;
    }
    if (singleShutdownTime.isValid()) {
        config.singleShutdownTime = singleShutdownTime;
    }
    if (singleWakeDate.isValid()) {
        config.singleWakeDate = singleWakeDate;
    }
    if (singleWakeTime.isValid()) {
        config.singleWakeTime = singleWakeTime;
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
        const QString shutdownStr = obj.value(QStringLiteral("shutdownTime")).toString();
        const auto parsedShutdown = QTime::fromString(shutdownStr, QStringLiteral("HH:mm"));
        if (parsedShutdown.isValid()) {
            entry.shutdownTime = parsedShutdown;
        }

        QString wakeKey = QStringLiteral("wakeTime");
        QString wakeStr = obj.value(wakeKey).toString();
        if (wakeStr.isEmpty()) {
            wakeStr = obj.value(QStringLiteral("time")).toString();
        }
        const auto parsedWake = QTime::fromString(wakeStr, QStringLiteral("HH:mm"));
        if (parsedWake.isValid()) {
            entry.wakeTime = parsedWake;
        }
        overrides.insert(day, entry);
    }

    for (auto &entry : config.weekly) {
        const int day = static_cast<int>(entry.day);
        if (overrides.contains(day)) {
            entry = overrides.value(day);
        }
    }

    const auto sessionObj = root.value(QStringLiteral("session")).toObject();
    if (!sessionObj.isEmpty()) {
        config.session.user = sessionObj.value(QStringLiteral("user")).toString(config.session.user);
        config.session.display = sessionObj.value(QStringLiteral("display")).toString(config.session.display);
        config.session.xdgRuntimeDir = sessionObj.value(QStringLiteral("xdgRuntimeDir")).toString(config.session.xdgRuntimeDir);
        config.session.dbusAddress = sessionObj.value(QStringLiteral("dbusAddress")).toString(config.session.dbusAddress);
    }

    return config;
}

QByteArray ConfigRepository::serialize(const AppConfig &config) const {
    QJsonObject root;
    root.insert(QStringLiteral("singleShutdownDate"), config.singleShutdownDate.toString(Qt::ISODate));
    root.insert(QStringLiteral("singleShutdownTime"), config.singleShutdownTime.toString(Qt::ISODate));
    root.insert(QStringLiteral("singleDate"), config.singleWakeDate.toString(Qt::ISODate));
    root.insert(QStringLiteral("singleTime"), config.singleWakeTime.toString(Qt::ISODate));
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
        obj.insert(QStringLiteral("shutdownTime"), formatTime(entry.shutdownTime));
        obj.insert(QStringLiteral("wakeTime"), formatTime(entry.wakeTime));
        weeklyArray.append(obj);
    }
    root.insert(QStringLiteral("weekly"), weeklyArray);

    QJsonObject sessionObj;
    sessionObj.insert(QStringLiteral("user"), config.session.user);
    sessionObj.insert(QStringLiteral("display"), config.session.display);
    sessionObj.insert(QStringLiteral("xdgRuntimeDir"), config.session.xdgRuntimeDir);
    sessionObj.insert(QStringLiteral("dbusAddress"), config.session.dbusAddress);
    root.insert(QStringLiteral("session"), sessionObj);

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}
