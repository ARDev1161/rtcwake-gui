#pragma once

#include "RtcWakeController.h"

#include <QDateTime>
#include <QString>

namespace SummaryWriter {
bool write(const QString &homeDir, const QDateTime &targetLocal, PowerAction action);
}
