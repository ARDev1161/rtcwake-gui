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
    config.singleDate = QDate::currentDate().addDays(1);
    config.singleTime = QTime(6, 0);
    config.actionId = static_cast<int>(PowerAction::PowerOff);
    for (auto &entry : config.weekly) {
        entry.enabled = false;
    }

    QDateTime now = QDateTime::currentDateTime();
    QDateTime target;
    PowerAction action;
    QVERIFY(SchedulePlanner::nextEvent(config, now, target, action));
    QCOMPARE(action, PowerAction::PowerOff);
    QCOMPARE(target.date(), config.singleDate);
    QCOMPARE(target.time(), config.singleTime);
}

void SchedulePlannerTest::falls_back_to_weekly() {
    AppConfig config;
    config.singleDate = QDate::currentDate().addDays(-1); // past
    config.singleTime = QTime(6, 0);
    config.actionId = static_cast<int>(PowerAction::SuspendToRam);
    for (auto &entry : config.weekly) {
        entry.enabled = (entry.day == Qt::Wednesday);
        entry.time = QTime(9, 30);
    }

    QDateTime now(QDate::currentDate(), QTime(7, 0), QTimeZone::systemTimeZone());
    QDateTime target;
    PowerAction action;
    QVERIFY(SchedulePlanner::nextEvent(config, now, target, action));
    QCOMPARE(action, PowerAction::SuspendToRam);
    QCOMPARE(target.time(), QTime(9, 30));
    QVERIFY(target > now);
}

void SchedulePlannerTest::skips_disabled() {
    AppConfig config;
    config.actionId = static_cast<int>(PowerAction::Hibernate);
    for (auto &entry : config.weekly) {
        entry.enabled = false;
    }
    config.weekly[0].enabled = true;
    config.weekly[0].day = Qt::Monday;
    config.weekly[0].time = QTime(10, 0);

    QDate base = QDate(2030, 1, 1); // Wednesday
    QDateTime now(base, QTime(12, 0), QTimeZone::systemTimeZone());
    QDateTime target;
    PowerAction action;
    QVERIFY(SchedulePlanner::nextEvent(config, now, target, action));
    QCOMPARE(target.date().dayOfWeek(), static_cast<int>(Qt::Monday));
    QCOMPARE(action, PowerAction::Hibernate);
}

QTEST_MAIN(SchedulePlannerTest)

#include "SchedulePlannerTest.moc"
