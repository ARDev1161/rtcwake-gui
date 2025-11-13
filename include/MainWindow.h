#pragma once

#include "PowerStateDetector.h"
#include "RtcWakeController.h"
#include "WarningBanner.h"

#include <QMainWindow>
#include <QVector>

#include <optional>

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
    /** Identifies which part of the UI initiated a schedule request. */
    enum class ScheduleOrigin {
        Single,
        Weekly
    };

    /** Helper pointers bound to the weekly table. */
    struct WeeklyRow {
        Qt::DayOfWeek day;
        QCheckBox *enabled {nullptr};
        QTimeEdit *timeEdit {nullptr};
    };

    /** Snapshot of a postponed request so it can be retried later. */
    struct PendingSchedule {
        QDateTime target;
        PowerAction action {PowerAction::None};
        ScheduleOrigin origin {ScheduleOrigin::Single};
    };

    /** Runtime configuration selected by the warning controls. */
    struct WarningConfig {
        bool enabled {false};
        QString message;
        int countdownSeconds {30};
        int snoozeMinutes {5};
    };

    void buildUi();
    QWidget *buildSingleTab();
    QWidget *buildWeeklyTab();
    void populateActionGroup(QVBoxLayout *layout);
    void connectSignals();

    void loadSettings();
    void saveSettings() const;

    PowerAction currentAction() const;
    WarningConfig currentWarning() const;

    void scheduleSingleWake();
    void scheduleNextFromWeekly();
    void handleScheduleRequest(const QDateTime &targetLocal, PowerAction action, ScheduleOrigin origin);
    void scheduleRetry(int minutesDelay);
    void persistSummary(const QDateTime &targetLocal, PowerAction action) const;

    void appendLog(const QString &line);

    QDateEdit *m_dateEdit {nullptr};
    QTimeEdit *m_timeEdit {nullptr};
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

    RtcWakeController *m_controller {nullptr};
    std::optional<PendingSchedule> m_postponed;
};
