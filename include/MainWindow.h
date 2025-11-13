#pragma once

#include "ConfigRepository.h"
#include "PowerStateDetector.h"

#include <QMainWindow>
#include <QVector>

class AnalogClockWidget;
class QButtonGroup;
class QCheckBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimeEdit;
class QVBoxLayout;

/**
 * @brief Feature-rich window that lets the user plan rtcwake events.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    /** Helper pointers bound to the weekly table. */
    struct WeeklyRow {
        Qt::DayOfWeek day;
        QCheckBox *enabled {nullptr};
        QTimeEdit *shutdownEdit {nullptr};
        QTimeEdit *wakeEdit {nullptr};
    };

    void buildUi();
    QWidget *buildSingleTab();
    QWidget *buildWeeklyTab();
    QWidget *buildSettingsTab();
    void populateActionGroup(QVBoxLayout *layout);
    void connectSignals();

    void loadSettings();
    void saveSettings(const QString &reason);
    void collectUiIntoConfig();
    void applyConfigToUi();

    PowerAction currentAction() const;

    void scheduleSingleWake();
    void scheduleNextFromWeekly();

    void appendLog(const QString &line);
    WeeklyEntry *weeklyConfig(Qt::DayOfWeek day);

    QDateEdit *m_dateEdit {nullptr};
    QTimeEdit *m_timeEdit {nullptr};
    QDateEdit *m_shutdownDateEdit {nullptr};
    QTimeEdit *m_shutdownTimeEdit {nullptr};
    AnalogClockWidget *m_clockWidget {nullptr};
    QPlainTextEdit *m_log {nullptr};
    QLabel *m_nextSummary {nullptr};

    QButtonGroup *m_actionGroup {nullptr};
    QVector<PowerStateDetector::Option> m_actionOptions;

    QCheckBox *m_warningEnabled {nullptr};
    QLineEdit *m_warningMessage {nullptr};
    QSpinBox *m_warningCountdown {nullptr};
    QSpinBox *m_warningSnooze {nullptr};

    QTableWidget *m_scheduleTable {nullptr};
    QVector<WeeklyRow> m_weeklyRows;

    ConfigRepository m_configRepo;
    AppConfig m_config;
};
