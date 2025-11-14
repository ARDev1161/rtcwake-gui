#pragma once

#include <QDialog>
#include <QTimer>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSize>
#include <memory>

class QLabel;
class QPushButton;
class QMediaPlayer;
class QMediaPlaylist;
/**
 * @brief Modal dialog that counts down before executing the selected power action.
 */
class WarningBanner : public QDialog {
    Q_OBJECT

public:
    enum class Theme {
        Crimson,
        Amber,
        Emerald,
        Slate
    };

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

    /**
     * @brief Configure the optional audio alert.
     */
    void setSoundOptions(bool enabled, const QString &filePath, int volumePercent);

    /**
     * @brief Configure colors and geometry.
     */
    void setVisualOptions(Theme theme, bool fullscreen, const QSize &customSize);

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
    bool m_soundEnabled {false};
    QString m_soundFile;
    int m_soundVolume {70};
    std::unique_ptr<QMediaPlayer> m_mediaPlayer;
    std::unique_ptr<QMediaPlaylist> m_mediaPlaylist;
    Theme m_theme {Theme::Crimson};
    bool m_fullscreen {false};
    QSize m_customSize {640, 360};

    void startSound();
    void stopSound();
    void applyTheme();
    void applyGeometry();
};
