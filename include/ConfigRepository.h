#pragma once

#include "AppConfig.h"

#include <QString>
#include <QByteArray>

/**
 * @brief Handles serialization of AppConfig to disk.
 */
class ConfigRepository {
public:
    AppConfig load() const;
    bool save(const AppConfig &config) const;
    QString configPath() const;

private:
    AppConfig parse(const QByteArray &json) const;
    QByteArray serialize(const AppConfig &config) const;
};
