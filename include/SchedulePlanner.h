#pragma once

#include "AppConfig.h"

#include <QDateTime>

namespace SchedulePlanner {

struct Event {
    QDateTime shutdown;
    QDateTime wake;
    PowerAction action {PowerAction::None};
};

bool nextEvent(const AppConfig &config, const QDateTime &now, Event &event);

}
