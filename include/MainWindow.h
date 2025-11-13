#pragma once

#include <QMainWindow>

class AnalogClockWidget;

/**
 * @brief Main window placeholder that will host the rtcwake GUI.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();

    AnalogClockWidget *m_clockWidget {nullptr};
};
