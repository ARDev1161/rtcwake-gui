#pragma once

#include "AppConfig.h"

#include <QString>
#include <QByteArray>

/**
 * @brief Handles serialization of AppConfig to disk.
 */
class ConfigRepository {
public:
    ConfigRepository();
    explicit ConfigRepository(QString explicitPath);

    AppConfig load() const;
    bool save(const AppConfig &config) const;
    QString configPath() const;

private:
    QString resolvedPath() const;
    AppConfig parse(const QByteArray &json) const;
    QByteArray serialize(const AppConfig &config) const;

    QString m_explicitPath;
};
