#include "PowerStateDetector.h"

#include <QFile>
#include <QStringList>

namespace {
QStringList readTokens(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QString data = QString::fromUtf8(file.readAll());
    QString normalized = data;
    normalized.replace('\n', ' ');
    return normalized.split(' ', Qt::SkipEmptyParts);
}
}

QVector<PowerStateDetector::Option> PowerStateDetector::detect() const {
    const QStringList tokens = readTokens(QStringLiteral("/sys/power/state"));
    const bool freeze = tokens.contains(QStringLiteral("freeze"));
    const bool mem = tokens.contains(QStringLiteral("mem"));
    const bool disk = tokens.contains(QStringLiteral("disk"));

    QVector<Option> options;
    options.reserve(5);

    options.push_back({PowerAction::None,
                       QObject::tr("Keep running"),
                       QObject::tr("Only program the RTC alarm"),
                       true});

    options.push_back({PowerAction::SuspendToIdle,
                       QObject::tr("Suspend to idle"),
                       QObject::tr("Low-power state without flushing RAM"),
                       freeze});

    options.push_back({PowerAction::SuspendToRam,
                       QObject::tr("Suspend to RAM"),
                       QObject::tr("Traditional sleep (S3)"),
                       mem});

    options.push_back({PowerAction::Hibernate,
                       QObject::tr("Hibernate"),
                       QObject::tr("Save memory to disk and power off"),
                       disk});

    options.push_back({PowerAction::PowerOff,
                       QObject::tr("Power off"),
                       QObject::tr("Shut down immediately"),
                       true});

    return options;
}
