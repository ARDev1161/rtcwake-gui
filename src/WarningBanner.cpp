#include "WarningBanner.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QUrl>
#include <QDir>
#include <algorithm>

namespace {
QUrl soundUrlFromPath(const QString &path) {
    if (path.startsWith(QStringLiteral("qrc:/"))) {
        return QUrl(path);
    }
    if (path.startsWith(QStringLiteral(":/"))) {
        return QUrl(QStringLiteral("qrc%1").arg(path));
    }
    QString normalized = path;
    if (normalized.startsWith(QStringLiteral("~/"))) {
        normalized.replace(0, 1, QDir::homePath());
    }
    return QUrl::fromLocalFile(QDir::cleanPath(normalized));
}

struct ThemeColors {
    QString background;
    QString text;
    QString accent;
    QString accentHover;
    QString accentDisabled;
};

ThemeColors paletteForTheme(WarningBanner::Theme theme) {
    switch (theme) {
    case WarningBanner::Theme::Amber:
        return {QStringLiteral("#ffb300"), QStringLiteral("#1b1b1b"),
                QStringLiteral("#ff8f00"), QStringLiteral("#ff6f00"), QStringLiteral("#ffd54f")};
    case WarningBanner::Theme::Emerald:
        return {QStringLiteral("#1b5e20"), QStringLiteral("#f1f8e9"),
                QStringLiteral("#2e7d32"), QStringLiteral("#43a047"), QStringLiteral("#81c784")};
    case WarningBanner::Theme::Slate:
        return {QStringLiteral("#263238"), QStringLiteral("#eceff1"),
                QStringLiteral("#37474f"), QStringLiteral("#455a64"), QStringLiteral("#90a4ae")};
    case WarningBanner::Theme::Crimson:
    default:
        return {QStringLiteral("#b71c1c"), QStringLiteral("#ffeaea"),
                QStringLiteral("#d32f2f"), QStringLiteral("#f44336"), QStringLiteral("#ef9a9a")};
    }
}
}

WarningBanner::WarningBanner(const QString &message, int countdownSeconds, QWidget *parent)
    : QDialog(parent), m_secondsLeft(countdownSeconds) {
    setWindowTitle(tr("Pending power action"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setWordWrap(true);

    m_timerLabel = new QLabel(this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold;"));

    auto *buttons = new QDialogButtonBox(this);
    m_applyButton = buttons->addButton(tr("Apply now"), QDialogButtonBox::AcceptRole);
    m_postponeButton = buttons->addButton(tr("Delay"), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Cancel);

    layout->addWidget(m_messageLabel);
    layout->addWidget(m_timerLabel);
    layout->addWidget(buttons);

    connect(m_applyButton, &QPushButton::clicked, this, [this]() {
        m_result = Result::ApplyNow;
        accept();
    });
    connect(m_postponeButton, &QPushButton::clicked, this, [this]() {
        m_result = Result::Postpone;
        accept();
    });
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, [this]() {
        m_result = Result::Cancelled;
        reject();
    });
    connect(&m_timer, &QTimer::timeout, this, &WarningBanner::handleTick);
}

WarningBanner::Result WarningBanner::execWithCountdown() {
    applyTheme();
    applyGeometry();
    m_timer.start(1000);
    handleTick();
    startSound();
    const int dialogResult = exec();
    m_timer.stop();
    stopSound();
    if (dialogResult == QDialog::Rejected && m_result == Result::Cancelled) {
        return Result::Cancelled;
    }
    return m_result;
}

void WarningBanner::handleTick() {
    m_timerLabel->setText(tr("Action in %1 s").arg(m_secondsLeft));
    if (m_secondsLeft <= 0) {
        m_result = Result::ApplyNow;
        accept();
        return;
    }
    --m_secondsLeft;
}

void WarningBanner::setSoundOptions(bool enabled, const QString &filePath, int volumePercent) {
    m_soundEnabled = enabled;
    m_soundFile = filePath;
    m_soundVolume = volumePercent;
}

void WarningBanner::setVisualOptions(WarningBanner::Theme theme, bool fullscreen, const QSize &customSize) {
    m_theme = theme;
    m_fullscreen = fullscreen;
    if (customSize.isValid()) {
        m_customSize = customSize;
    }
}

void WarningBanner::startSound() {
    if (!m_soundEnabled) {
        return;
    }
    stopSound();
    const QString source = m_soundFile.isEmpty() ? QStringLiteral(":/rtcwake/sounds/chime.wav") : m_soundFile;
    const QUrl url = soundUrlFromPath(source);
    const QString localPath = url.isLocalFile() ? url.toLocalFile() : QString();
    const bool useSoundEffect = (url.scheme() == QStringLiteral("qrc")
                                 || (!localPath.isEmpty() && localPath.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive)));
    const int clamped = std::clamp(m_soundVolume, 0, 100);
    if (useSoundEffect) {
        auto effect = std::make_unique<QSoundEffect>(this);
        effect->setSource(url);
        effect->setVolume(static_cast<qreal>(clamped) / 100.0);
        effect->setLoopCount(QSoundEffect::Infinite);
        effect->play();
        m_soundEffect = std::move(effect);
        return;
    }

    auto playlist = std::make_unique<QMediaPlaylist>(this);
    playlist->addMedia(url);
    playlist->setPlaybackMode(QMediaPlaylist::Loop);

    auto player = std::make_unique<QMediaPlayer>(this);
    player->setPlaylist(playlist.get());
    player->setVolume(clamped);
    player->play();

    m_mediaPlaylist = std::move(playlist);
    m_mediaPlayer = std::move(player);
}

void WarningBanner::stopSound() {
    if (m_soundEffect) {
        m_soundEffect->stop();
        m_soundEffect.reset();
    }
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
        m_mediaPlayer.reset();
    }
    if (m_mediaPlaylist) {
        m_mediaPlaylist.reset();
    }
}

void WarningBanner::applyTheme() {
    const ThemeColors colors = paletteForTheme(m_theme);
    const QString style = QStringLiteral(R"(
        QDialog {
            background-color: %1;
        }
        QLabel {
            color: %2;
        }
        QPushButton {
            background-color: %3;
            color: %2;
            border: none;
            padding: 8px 16px;
            font-weight: bold;
            border-radius: 6px;
        }
        QPushButton:hover:!disabled {
            background-color: %4;
        }
        QPushButton:disabled {
            background-color: %5;
            color: rgba(0, 0, 0, 0.4);
        }
    )").arg(colors.background, colors.text, colors.accent, colors.accentHover, colors.accentDisabled);
    setStyleSheet(style);
    if (m_timerLabel) {
        m_timerLabel->setStyleSheet(QStringLiteral("font-size: 28px; font-weight: 700;"));
    }
}

void WarningBanner::applyGeometry() {
    if (m_fullscreen) {
        setWindowState(Qt::WindowFullScreen);
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        if (m_customSize.isValid()) {
            resize(m_customSize);
        }
    }
}
