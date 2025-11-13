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
    config.singleDate = QDate(2035, 5, 25);
    config.singleTime = QTime(6, 45);
    config.actionId = static_cast<int>(PowerAction::Hibernate);
    config.warning.enabled = true;
    config.warning.message = QStringLiteral("Test message");
    config.warning.countdownSeconds = 42;
    config.warning.snoozeMinutes = 3;

    for (auto &entry : config.weekly) {
        entry.enabled = (entry.day == Qt::Monday || entry.day == Qt::Friday);
        entry.time = QTime(8 + static_cast<int>(entry.day), 15);
    }

    QVERIFY(repo.save(config));

    AppConfig loaded = repo.load();
    QCOMPARE(loaded.singleDate, config.singleDate);
    QCOMPARE(loaded.singleTime, config.singleTime);
    QCOMPARE(loaded.actionId, config.actionId);
    QCOMPARE(loaded.warning.enabled, config.warning.enabled);
    QCOMPARE(loaded.warning.message, config.warning.message);
    QCOMPARE(loaded.warning.countdownSeconds, config.warning.countdownSeconds);
    QCOMPARE(loaded.warning.snoozeMinutes, config.warning.snoozeMinutes);

    for (int i = 0; i < config.weekly.size(); ++i) {
        QCOMPARE(static_cast<int>(loaded.weekly.at(i).day), static_cast<int>(config.weekly.at(i).day));
        QCOMPARE(loaded.weekly.at(i).enabled, config.weekly.at(i).enabled);
        QCOMPARE(loaded.weekly.at(i).time, config.weekly.at(i).time);
    }
}

QTEST_MAIN(ConfigRepositoryTest)

#include "ConfigRepositoryTest.moc"
