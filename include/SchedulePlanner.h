#pragma once

#include "AppConfig.h"

#include <QDateTime>

namespace SchedulePlanner {

bool nextEvent(const AppConfig &config, const QDateTime &now, QDateTime &targetLocal, PowerAction &action);

}
