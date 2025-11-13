#include "SummaryWriter.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>

namespace SummaryWriter {

bool write(const QString &homeDir, const QDateTime &targetLocal, PowerAction action) {
    if (homeDir.isEmpty()) {
        return false;
    }

    QDir base(homeDir);
    const QString dirPath = base.filePath(QStringLiteral(".local/share/rtcwake-gui"));

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
