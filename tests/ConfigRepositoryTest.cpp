#include <QtTest>

#include "ConfigRepository.h"
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDebug>

class ConfigRepositoryTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void saveAndLoad();
};

void ConfigRepositoryTest::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
}

void ConfigRepositoryTest::saveAndLoad() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());
    qputenv("HOME", dir.path().toUtf8());
    QStandardPaths::setTestModeEnabled(true); // refresh paths with new env

    ConfigRepository repo;
    const QString path = repo.configPath();
    qInfo() << "config path" << path;
    QVERIFY(!path.isEmpty());
    AppConfig config;
    config.singleShutdownDate = QDate(2035, 5, 24);
    config.singleShutdownTime = QTime(22, 30);
    config.singleWakeDate = QDate(2035, 5, 25);
    config.singleWakeTime = QTime(6, 45);
    config.actionId = static_cast<int>(PowerAction::Hibernate);
    config.warning.enabled = true;
    config.warning.message = QStringLiteral("Test message");
    config.warning.countdownSeconds = 42;
    config.warning.snoozeMinutes = 3;
    config.warning.soundEnabled = true;
    config.warning.soundFile = QStringLiteral("/tmp/custom.wav");
    config.warning.soundVolume = 80;
    config.warning.theme = QStringLiteral("emerald");
    config.warning.fullscreen = true;
    config.warning.width = 1280;
    config.warning.height = 720;
    config.session.user = QStringLiteral("tester");
    config.session.display = QStringLiteral(":1");
    config.session.xdgRuntimeDir = QStringLiteral("/run/user/999");
    config.session.dbusAddress = QStringLiteral("unix:path=/run/user/999/bus");

    for (auto &entry : config.weekly) {
        entry.enabled = (entry.day == Qt::Monday || entry.day == Qt::Friday);
        entry.shutdownTime = QTime(22, 15);
        entry.wakeTime = QTime(6 + static_cast<int>(entry.day), 30);
    }

    QVERIFY(repo.save(config));

    AppConfig loaded = repo.load();
    QCOMPARE(loaded.singleShutdownDate, config.singleShutdownDate);
    QCOMPARE(loaded.singleShutdownTime, config.singleShutdownTime);
    QCOMPARE(loaded.singleWakeDate, config.singleWakeDate);
    QCOMPARE(loaded.singleWakeTime, config.singleWakeTime);
    QCOMPARE(loaded.actionId, config.actionId);
    QCOMPARE(loaded.warning.enabled, config.warning.enabled);
    QCOMPARE(loaded.warning.message, config.warning.message);
    QCOMPARE(loaded.warning.countdownSeconds, config.warning.countdownSeconds);
    QCOMPARE(loaded.warning.snoozeMinutes, config.warning.snoozeMinutes);
    QCOMPARE(loaded.warning.soundEnabled, config.warning.soundEnabled);
    QCOMPARE(loaded.warning.soundFile, config.warning.soundFile);
    QCOMPARE(loaded.warning.soundVolume, config.warning.soundVolume);
    QCOMPARE(loaded.warning.theme, config.warning.theme);
    QCOMPARE(loaded.warning.fullscreen, config.warning.fullscreen);
    QCOMPARE(loaded.warning.width, config.warning.width);
    QCOMPARE(loaded.warning.height, config.warning.height);
    QCOMPARE(loaded.session.user, config.session.user);
    QCOMPARE(loaded.session.display, config.session.display);
    QCOMPARE(loaded.session.xdgRuntimeDir, config.session.xdgRuntimeDir);
    QCOMPARE(loaded.session.dbusAddress, config.session.dbusAddress);

    for (int i = 0; i < config.weekly.size(); ++i) {
        QCOMPARE(static_cast<int>(loaded.weekly.at(i).day), static_cast<int>(config.weekly.at(i).day));
        QCOMPARE(loaded.weekly.at(i).enabled, config.weekly.at(i).enabled);
        QCOMPARE(loaded.weekly.at(i).shutdownTime, config.weekly.at(i).shutdownTime);
        QCOMPARE(loaded.weekly.at(i).wakeTime, config.weekly.at(i).wakeTime);
    }
}

QTEST_MAIN(ConfigRepositoryTest)

#include "ConfigRepositoryTest.moc"
