#pragma once

#include "RtcWakeController.h"

#include <QVector>

/**
 * @brief Discovers which suspend/hibernate states are supported by the kernel.
 */
class PowerStateDetector {
public:
    struct Option {
        PowerAction action;
        QString label;
        QString description;
        bool available;
    };

    QVector<Option> detect() const;
};
