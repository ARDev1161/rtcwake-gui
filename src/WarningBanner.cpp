#include "WarningBanner.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSoundEffect>
#include <QUrl>
#include <algorithm>

namespace {
QUrl soundUrlFromPath(const QString &path) {
    if (path.startsWith(QStringLiteral("qrc:/"))) {
        return QUrl(path);
    }
    if (path.startsWith(QStringLiteral(":/"))) {
        return QUrl(QStringLiteral("qrc%1").arg(path));
    }
    return QUrl::fromLocalFile(path);
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

void WarningBanner::startSound() {
    if (!m_soundEnabled) {
        return;
    }
    const QString source = m_soundFile.isEmpty() ? QStringLiteral(":/rtcwake/sounds/chime.wav") : m_soundFile;
    auto effect = std::make_unique<QSoundEffect>(this);
    effect->setSource(soundUrlFromPath(source));
    const int clamped = std::clamp(m_soundVolume, 0, 100);
    effect->setVolume(static_cast<qreal>(clamped) / 100.0);
    effect->setLoopCount(QSoundEffect::Infinite);
    effect->play();
    m_soundEffect = std::move(effect);
}

void WarningBanner::stopSound() {
    if (m_soundEffect) {
        m_soundEffect->stop();
        m_soundEffect.reset();
    }
}
