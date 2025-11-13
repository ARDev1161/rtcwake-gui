#include "SummaryWriter.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QLocale>

namespace SummaryWriter {

bool write(const QDateTime &targetLocal, PowerAction action) {
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/rtcwake-gui");
    if (dirPath.isEmpty()) {
        return false;
    }

    QDir dir;
    if (!dir.mkpath(dirPath)) {
        return false;
    }

    QFile file(dirPath + QStringLiteral("/next-wake.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("timestamp"), static_cast<qint64>(targetLocal.toSecsSinceEpoch()));
    payload.insert(QStringLiteral("localTime"), targetLocal.toString(Qt::ISODate));
    payload.insert(QStringLiteral("friendly"), QLocale().toString(targetLocal, QLocale::LongFormat));
    payload.insert(QStringLiteral("mode"), RtcWakeController::rtcwakeMode(action));
    payload.insert(QStringLiteral("action"), RtcWakeController::actionLabel(action));

    QJsonDocument doc(payload);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.write("\n");
    return true;
}

} // namespace SummaryWriter
