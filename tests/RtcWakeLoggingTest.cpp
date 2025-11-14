#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "RtcWakeDaemon.h"

class RtcWakeLoggingTest : public QObject {
    Q_OBJECT

private slots:
    void writes_sanitized_entries();
};

void RtcWakeLoggingTest::writes_sanitized_entries() {
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "Temporary directory must be available for log writing test");

    RtcWakeDaemon::Options options;
    options.targetHome = dir.path();
    options.configPath = QString();
    options.warningApp = QString();
    RtcWakeDaemon daemon(options);

    daemon.m_rtcwakeLogPath = dir.filePath(QStringLiteral("log.txt"));

    QList<QPair<QString, QString>> fields {
        {QStringLiteral("key"), QStringLiteral("value with\nnewline")},
        {QStringLiteral("empty"), QString()}
    };
    daemon.appendPersistentLog(QStringLiteral("test\ncategory"), fields);

    QFile logFile(daemon.m_rtcwakeLogPath);
    QVERIFY(logFile.exists());
    QVERIFY(logFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString contents = QString::fromUtf8(logFile.readAll()).trimmed();

    QVERIFY(contents.contains(QStringLiteral("category=\"test category\"")));
    QVERIFY(contents.contains(QStringLiteral("key=\"value with newline\"")));
    QVERIFY(contents.contains(QStringLiteral("empty=\"\"")));
}

QTEST_MAIN(RtcWakeLoggingTest)

#include "RtcWakeLoggingTest.moc"
