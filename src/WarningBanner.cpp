#include "WarningBanner.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

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
    const int dialogResult = exec();
    m_timer.stop();
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
