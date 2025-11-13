#include <QtTest>

#include "SchedulePlanner.h"
#include <QTimeZone>

class SchedulePlannerTest : public QObject {
    Q_OBJECT

private slots:
    void picks_single_future();
    void falls_back_to_weekly();
    void skips_disabled();
};

void SchedulePlannerTest::picks_single_future() {
    AppConfig config;
    config.singleShutdownDate = QDate(2030, 1, 1);
    config.singleShutdownTime = QTime(22, 0);
    config.singleWakeDate = QDate(2030, 1, 2);
    config.singleWakeTime = QTime(6, 0);
    config.actionId = static_cast<int>(PowerAction::PowerOff);
    for (auto &entry : config.weekly) {
        entry.enabled = false;
    }

    QDateTime now(QDate(2030, 1, 1), QTime(20, 0), QTimeZone::systemTimeZone());
    SchedulePlanner::Event event;
    QVERIFY(SchedulePlanner::nextEvent(config, now, event));
    QCOMPARE(event.action, PowerAction::PowerOff);
    QCOMPARE(event.shutdown, QDateTime(QDate(2030, 1, 1), QTime(22, 0), QTimeZone::systemTimeZone()));
    QCOMPARE(event.wake, QDateTime(QDate(2030, 1, 2), QTime(6, 0), QTimeZone::systemTimeZone()));
}

void SchedulePlannerTest::falls_back_to_weekly() {
    AppConfig config;
    config.singleShutdownDate = QDate::currentDate().addDays(-2);
    config.singleShutdownTime = QTime(22, 0);
    config.singleWakeDate = QDate::currentDate().addDays(-1);
    config.singleWakeTime = QTime(6, 0);
    config.actionId = static_cast<int>(PowerAction::SuspendToRam);
    for (auto &entry : config.weekly) {
        entry.enabled = (entry.day == Qt::Wednesday);
        entry.shutdownTime = QTime(23, 0);
        entry.wakeTime = QTime(7, 0);
    }

    QDateTime now(QDate(2030, 1, 1), QTime(20, 0), QTimeZone::systemTimeZone()); // Wednesday eve
    SchedulePlanner::Event event;
    QVERIFY(SchedulePlanner::nextEvent(config, now, event));
    QCOMPARE(event.action, PowerAction::SuspendToRam);
    QVERIFY(event.shutdown > now);
    QVERIFY(event.wake > event.shutdown);
    QCOMPARE(event.shutdown.time(), QTime(23, 0));
    QCOMPARE(event.wake.time(), QTime(7, 0));
}

void SchedulePlannerTest::skips_disabled() {
    AppConfig config;
    config.actionId = static_cast<int>(PowerAction::Hibernate);
    for (auto &entry : config.weekly) {
        entry.enabled = false;
    }
    config.weekly[0].enabled = true;
    config.weekly[0].day = Qt::Monday;
    config.weekly[0].shutdownTime = QTime(21, 0);
    config.weekly[0].wakeTime = QTime(6, 0);

    QDate base = QDate(2030, 1, 1); // Wednesday
    QDateTime now(base, QTime(12, 0), QTimeZone::systemTimeZone());
    SchedulePlanner::Event event;
    QVERIFY(SchedulePlanner::nextEvent(config, now, event));
    QCOMPARE(event.shutdown.date().dayOfWeek(), static_cast<int>(Qt::Monday));
    QCOMPARE(event.action, PowerAction::Hibernate);
}

QTEST_MAIN(SchedulePlannerTest)

#include "SchedulePlannerTest.moc"
