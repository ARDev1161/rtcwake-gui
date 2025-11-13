#pragma once

#include "RtcWakeController.h"

#include <QDateTime>

namespace SummaryWriter {
bool write(const QDateTime &targetLocal, PowerAction action);
}
