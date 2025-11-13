#pragma once

#include "AppConfig.h"
#include "ConfigRepository.h"
#include "RtcWakeController.h"

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

/**
 * @brief Background worker that keeps the RTC alarm armed according to AppConfig.
 */
class RtcWakeDaemon : public QObject {
    Q_OBJECT

public:
    explicit RtcWakeDaemon(QObject *parent = nullptr);

    void start();

private slots:
    void handleConfigChanged();
    void handlePeriodic();

private:
    void watchConfig();
    void reloadConfig();
    void planNext();
    void log(const QString &message) const;

    ConfigRepository m_repo;
    AppConfig m_config;
    RtcWakeController m_controller;
    QFileSystemWatcher m_watcher;
    QTimer m_periodic;
    QDateTime m_lastArmed;
};
