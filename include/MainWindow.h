#pragma once

#include "ConfigRepository.h"
#include "PowerStateDetector.h"

#include <QMainWindow>
#include <QVector>

class AnalogClockWidget;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSlider;
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
    QWidget *buildLogsTab();
    void populateActionGroup(QVBoxLayout *layout);
    void connectSignals();
    void updateSoundControls();
    void updateBannerSizeControls();

    void loadSettings();
    void saveSettings(const QString &reason);
    void collectUiIntoConfig();
    void applyConfigToUi();
    QString logFilePath() const;
    void refreshLogViewer();

    PowerAction currentAction() const;

    void scheduleSingleWake();
    void scheduleNextFromWeekly();

    WeeklyEntry *weeklyConfig(Qt::DayOfWeek day);

    QDateEdit *m_dateEdit {nullptr};
    QTimeEdit *m_timeEdit {nullptr};
    QDateEdit *m_shutdownDateEdit {nullptr};
    QTimeEdit *m_shutdownTimeEdit {nullptr};
    AnalogClockWidget *m_clockWidget {nullptr};
    QPlainTextEdit *m_logViewer {nullptr};
    QLabel *m_nextSummary {nullptr};

    QButtonGroup *m_actionGroup {nullptr};
    QVector<PowerStateDetector::Option> m_actionOptions;

    QCheckBox *m_warningEnabled {nullptr};
    QLineEdit *m_warningMessage {nullptr};
    QSpinBox *m_warningCountdown {nullptr};
    QSpinBox *m_warningSnooze {nullptr};
    QCheckBox *m_soundEnabled {nullptr};
    QLineEdit *m_soundFile {nullptr};
    QPushButton *m_soundBrowse {nullptr};
    QPushButton *m_soundReset {nullptr};
    QSlider *m_soundVolume {nullptr};
    QLabel *m_soundVolumeLabel {nullptr};
    QComboBox *m_themeCombo {nullptr};
    QCheckBox *m_fullscreenBanner {nullptr};
    QSpinBox *m_bannerWidth {nullptr};
    QSpinBox *m_bannerHeight {nullptr};

    QTableWidget *m_scheduleTable {nullptr};
    QVector<WeeklyRow> m_weeklyRows;

    ConfigRepository m_configRepo;
    AppConfig m_config;
};
