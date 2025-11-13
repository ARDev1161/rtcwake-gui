#pragma once

#include "RtcWakeController.h"

#include <QVector>

/**
 * @brief Discovers which suspend/hibernate states are supported by the kernel.
 */
class PowerStateDetector {
public:
    /** Single action option for the main UI. */
    struct Option {
        PowerAction action;
        QString label;
        QString description;
        bool available;
    };

    /**
     * @brief Inspect the kernel capabilities and expose radio button metadata.
     */
    QVector<Option> detect() const;
};
