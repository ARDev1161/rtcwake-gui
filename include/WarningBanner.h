#pragma once

#include <QDialog>
#include <QTimer>

class QLabel;
class QPushButton;

/**
 * @brief Modal dialog that counts down before executing the selected power action.
 */
class WarningBanner : public QDialog {
    Q_OBJECT

public:
    /** Modal outcome communicated back to the caller. */
    enum class Result {
        ApplyNow,
        Postpone,
        Cancelled
    };

    WarningBanner(const QString &message, int countdownSeconds, QWidget *parent = nullptr);

    /**
     * @brief Show the dialog and tick down the timer automatically.
     * @return User choice once the dialog closes.
     */
    Result execWithCountdown();

private slots:
    void handleTick();

private:
    int m_secondsLeft;
    QLabel *m_messageLabel {nullptr};
    QLabel *m_timerLabel {nullptr};
    QPushButton *m_applyButton {nullptr};
    QPushButton *m_postponeButton {nullptr};
    Result m_result {Result::Cancelled};
    QTimer m_timer;
};
