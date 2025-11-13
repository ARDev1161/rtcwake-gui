#include "MainWindow.h"

#include "AnalogClockWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateEdit>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimeEdit>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QtGlobal>
#include <algorithm>

namespace {
QString formatDateTime(const QDateTime &dt) {
    QLocale locale;
    return locale.toString(dt, QLocale::LongFormat);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_controller(new RtcWakeController(this)) {
    buildUi();
    connectSignals();
    loadSettings();
    m_clockWidget->setTime(m_timeEdit->time());
    setWindowIcon(QIcon(QStringLiteral(":/rtcwake/icons/clock.svg")));
    setWindowTitle(tr("rtcwake planner"));
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::buildUi() {
    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildSingleTab(), tr("Single wake"));
    tabs->addTab(buildWeeklyTab(), tr("Weekly schedule"));
    tabs->addTab(buildSettingsTab(), tr("Settings"));

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);

    m_nextSummary = new QLabel(tr("No wake scheduled yet."), this);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(tabs);
    layout->addWidget(m_nextSummary);
    layout->addWidget(new QLabel(tr("Activity log:"), this));
    layout->addWidget(m_log, 1);
    setCentralWidget(central);
    resize(900, 720);
}

QWidget *MainWindow::buildSingleTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    auto *timeBox = new QGroupBox(tr("Wake time"), tab);
    auto *timeLayout = new QGridLayout(timeBox);

    m_dateEdit = new QDateEdit(QDate::currentDate(), timeBox);
    m_dateEdit->setCalendarPopup(true);

    m_timeEdit = new QTimeEdit(QTime::currentTime().addSecs(900), timeBox);
    m_timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    m_timeEdit->setMinimumWidth(120);

    timeLayout->addWidget(new QLabel(tr("Date:"), timeBox), 0, 0);
    timeLayout->addWidget(m_dateEdit, 0, 1);
    timeLayout->addWidget(new QLabel(tr("Time:"), timeBox), 1, 0);
    timeLayout->addWidget(m_timeEdit, 1, 1);

    m_clockWidget = new AnalogClockWidget(timeBox);
    timeLayout->addWidget(m_clockWidget, 0, 2, 2, 1);

    layout->addWidget(timeBox);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto *scheduleButton = new QPushButton(tr("Schedule wake"), tab);
    buttonRow->addWidget(scheduleButton);
    layout->addLayout(buttonRow);

    connect(scheduleButton, &QPushButton::clicked, this, &MainWindow::scheduleSingleWake);

    return tab;
}

QWidget *MainWindow::buildWeeklyTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    m_scheduleTable = new QTableWidget(7, 3, tab);
    m_scheduleTable->setHorizontalHeaderLabels({tr("Enabled"), tr("Day"), tr("Time")});
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_scheduleTable->verticalHeader()->setVisible(false);

    const QList<Qt::DayOfWeek> days {
        Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday,
        Qt::Friday, Qt::Saturday, Qt::Sunday
    };

    QLocale locale;

    for (int row = 0; row < days.size(); ++row) {
        auto day = days.at(row);
        auto *enabled = new QCheckBox(tab);
        auto *timeEdit = new QTimeEdit(QTime(7, 30), tab);
        timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));

        auto *dayItem = new QTableWidgetItem(locale.dayName(day));
        dayItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_scheduleTable->setCellWidget(row, 0, enabled);
        m_scheduleTable->setItem(row, 1, dayItem);
        m_scheduleTable->setCellWidget(row, 2, timeEdit);

        m_weeklyRows.push_back({day, enabled, timeEdit});
    }

    layout->addWidget(m_scheduleTable);

    auto *hint = new QLabel(tr("Select multiple days to build a weekly alarm. "
                               "The nearest future occurrence will be scheduled."), tab);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *button = new QPushButton(tr("Schedule next weekly wake"), tab);
    layout->addWidget(button, 0, Qt::AlignRight);

    connect(button, &QPushButton::clicked, this, &MainWindow::scheduleNextFromWeekly);

    return tab;
}

QWidget *MainWindow::buildSettingsTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    auto *actionsBox = new QGroupBox(tr("Power action"), tab);
    auto *actionsLayout = new QVBoxLayout(actionsBox);
    populateActionGroup(actionsLayout);
    layout->addWidget(actionsBox);

    auto *warningBox = new QGroupBox(tr("Warning banner"), tab);
    auto *warningLayout = new QGridLayout(warningBox);
    m_warningEnabled = new QCheckBox(tr("Show warning before sleeping"), warningBox);
    m_warningMessage = new QLineEdit(tr("System will suspend soon. Save your work."), warningBox);
    m_warningCountdown = new QSpinBox(warningBox);
    m_warningCountdown->setRange(5, 600);
    m_warningCountdown->setValue(30);
    m_warningCountdown->setSuffix(tr(" s"));

    m_warningSnooze = new QSpinBox(warningBox);
    m_warningSnooze->setRange(1, 120);
    m_warningSnooze->setValue(5);
    m_warningSnooze->setSuffix(tr(" min"));

    warningLayout->addWidget(m_warningEnabled, 0, 0, 1, 2);
    warningLayout->addWidget(new QLabel(tr("Message:"), warningBox), 1, 0);
    warningLayout->addWidget(m_warningMessage, 1, 1);
    warningLayout->addWidget(new QLabel(tr("Countdown:"), warningBox), 2, 0);
    warningLayout->addWidget(m_warningCountdown, 2, 1);
    warningLayout->addWidget(new QLabel(tr("Snooze interval:"), warningBox), 3, 0);
    warningLayout->addWidget(m_warningSnooze, 3, 1);

    layout->addWidget(warningBox);
    layout->addStretch();

    return tab;
}

void MainWindow::populateActionGroup(QVBoxLayout *layout) {
    m_actionGroup = new QButtonGroup(this);
    m_actionGroup->setExclusive(true);

    PowerStateDetector detector;
    m_actionOptions = detector.detect();

    bool firstChecked = false;
    for (const auto &option : m_actionOptions) {
        auto *radio = new QRadioButton(option.label, this);
        radio->setToolTip(option.description);
        radio->setEnabled(option.available);
        layout->addWidget(radio);
        m_actionGroup->addButton(radio, static_cast<int>(option.action));
        if (!firstChecked && option.available) {
            radio->setChecked(true);
            firstChecked = true;
        }
    }
}

void MainWindow::connectSignals() {
    connect(m_timeEdit, &QTimeEdit::timeChanged, this, [this](const QTime &time) {
        m_clockWidget->setTime(time);
    });

    auto updateWarningUi = [this](bool enabled) {
        m_warningMessage->setEnabled(enabled);
        m_warningCountdown->setEnabled(enabled);
        m_warningSnooze->setEnabled(enabled);
    };
    connect(m_warningEnabled, &QCheckBox::toggled, this, updateWarningUi);
    updateWarningUi(m_warningEnabled->isChecked());
}

void MainWindow::loadSettings() {
    m_config = m_configRepo.load();

    m_dateEdit->setDate(m_config.singleDate);
    m_timeEdit->setTime(m_config.singleTime);

    if (auto *button = m_actionGroup->button(m_config.actionId)) {
        button->setChecked(true);
    }

    m_warningEnabled->setChecked(m_config.warning.enabled);
    m_warningMessage->setText(m_config.warning.message);
    m_warningCountdown->setValue(m_config.warning.countdownSeconds);
    m_warningSnooze->setValue(m_config.warning.snoozeMinutes);

    for (const auto &row : m_weeklyRows) {
        if (auto *entry = weeklyConfig(row.day)) {
            row.enabled->setChecked(entry->enabled);
            row.timeEdit->setTime(entry->time);
        } else {
            row.enabled->setChecked(false);
            row.timeEdit->setTime(QTime(7, 30));
        }
    }
}

void MainWindow::saveSettings() {
    m_config.singleDate = m_dateEdit->date();
    m_config.singleTime = m_timeEdit->time();
    m_config.actionId = m_actionGroup->checkedId();

    m_config.warning.enabled = m_warningEnabled->isChecked();
    m_config.warning.message = m_warningMessage->text();
    m_config.warning.countdownSeconds = m_warningCountdown->value();
    m_config.warning.snoozeMinutes = m_warningSnooze->value();

    for (const auto &row : m_weeklyRows) {
        if (auto *entry = weeklyConfig(row.day)) {
            entry->enabled = row.enabled->isChecked();
            entry->time = row.timeEdit->time();
        }
    }

    m_configRepo.save(m_config);
}

PowerAction MainWindow::currentAction() const {
    const int id = m_actionGroup->checkedId();
    if (id == -1) {
        return PowerAction::None;
    }
    return static_cast<PowerAction>(id);
}

MainWindow::WarningConfig MainWindow::currentWarning() const {
    WarningConfig config;
    config.enabled = m_warningEnabled->isChecked();
    config.message = m_warningMessage->text();
    config.countdownSeconds = m_warningCountdown->value();
    config.snoozeMinutes = m_warningSnooze->value();
    return config;
}

void MainWindow::scheduleSingleWake() {
    const QDate date = m_dateEdit->date();
    const QTime time = m_timeEdit->time();
    QDateTime target(date, time, QTimeZone::systemTimeZone());

    handleScheduleRequest(target, currentAction(), ScheduleOrigin::Single);
}

void MainWindow::scheduleNextFromWeekly() {
    QDateTime best;
    bool hasCandidate = false;
    const QDateTime now = QDateTime::currentDateTime();

    for (const auto &row : m_weeklyRows) {
        if (!row.enabled->isChecked()) {
            continue;
        }
        QDate candidateDate = now.date();
        int offset = static_cast<int>(row.day) - now.date().dayOfWeek();
        if (offset < 0) {
            offset += 7;
        }
        candidateDate = candidateDate.addDays(offset);
        QDateTime candidate(candidateDate, row.timeEdit->time(), now.timeZone());
        if (candidate <= now) {
            candidate = candidate.addDays(7);
        }
        if (!hasCandidate || candidate < best) {
            best = candidate;
            hasCandidate = true;
        }
    }

    if (!hasCandidate) {
        QMessageBox::information(this, tr("No schedule"), tr("Enable at least one weekday."));
        return;
    }

    handleScheduleRequest(best, currentAction(), ScheduleOrigin::Weekly);
}

void MainWindow::handleScheduleRequest(const QDateTime &targetLocal, PowerAction action, ScheduleOrigin origin) {
    const QDateTime now = QDateTime::currentDateTime();
    if (!targetLocal.isValid() || targetLocal <= now) {
        QMessageBox::warning(this, tr("Invalid time"), tr("Choose a future date and time."));
        return;
    }

    const QString originLabel = (origin == ScheduleOrigin::Single)
        ? tr("single entry")
        : tr("weekly plan");

    auto proceed = [this, targetLocal, action, originLabel]() {
        m_postponed.reset();
        auto result = m_controller->scheduleWake(targetLocal.toUTC(), action);
        if (result.success) {
            appendLog(tr("Scheduled %1 via %2 (%3)")
                          .arg(formatDateTime(targetLocal))
                          .arg(result.commandLine)
                          .arg(originLabel));
            m_nextSummary->setText(tr("Wake at %1 (%2)")
                                       .arg(formatDateTime(targetLocal))
                                       .arg(RtcWakeController::actionLabel(action)));
            persistSummary(targetLocal, action);
        } else {
            appendLog(tr("Scheduling failed: %1").arg(result.stdErr));
            QMessageBox::critical(this, tr("rtcwake error"),
                                  tr("Command failed:\n%1\n%2").arg(result.commandLine, result.stdErr));
        }
    };

    const auto warning = currentWarning();
    if (action != PowerAction::None && warning.enabled) {
        WarningBanner banner(warning.message, warning.countdownSeconds, this);
        const auto choice = banner.execWithCountdown();
        if (choice == WarningBanner::Result::ApplyNow) {
            proceed();
        } else if (choice == WarningBanner::Result::Postpone) {
            PendingSchedule pending {targetLocal, action, origin};
            m_postponed = pending;
            appendLog(tr("Power transition postponed for %1 minutes.").arg(warning.snoozeMinutes));
            scheduleRetry(warning.snoozeMinutes);
        } else {
            appendLog(tr("Scheduling cancelled by user."));
        }
        return;
    }

    proceed();
}

void MainWindow::scheduleRetry(int minutesDelay) {
    const int minutes = qMax(1, minutesDelay);
    const int ms = minutes * 60 * 1000;
    QTimer::singleShot(ms, this, [this]() {
        if (!m_postponed) {
            return;
        }
        auto pendingCopy = *m_postponed;
        m_postponed.reset();
        handleScheduleRequest(pendingCopy.target, pendingCopy.action, pendingCopy.origin);
    });
}

void MainWindow::appendLog(const QString &line) {
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_log->appendPlainText(QStringLiteral("[%1] %2").arg(stamp, line));
}

WeeklyEntry *MainWindow::weeklyConfig(Qt::DayOfWeek day) {
    auto it = std::find_if(m_config.weekly.begin(), m_config.weekly.end(), [day](const WeeklyEntry &entry) {
        return entry.day == day;
    });
    if (it == m_config.weekly.end()) {
        WeeklyEntry entry;
        entry.day = day;
        m_config.weekly.push_back(entry);
        return &m_config.weekly.last();
    }
    return &(*it);
}

void MainWindow::persistSummary(const QDateTime &targetLocal, PowerAction action) const {
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/rtcwake-gui");
    if (dirPath.isEmpty()) {
        return;
    }

    QDir dir;
    if (!dir.mkpath(dirPath)) {
        return;
    }

    QFile file(dirPath + QStringLiteral("/next-wake.json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("timestamp"), static_cast<qint64>(targetLocal.toSecsSinceEpoch()));
    payload.insert(QStringLiteral("localTime"), targetLocal.toString(Qt::ISODate));
    payload.insert(QStringLiteral("friendly"), formatDateTime(targetLocal));
    payload.insert(QStringLiteral("mode"), RtcWakeController::rtcwakeMode(action));
    payload.insert(QStringLiteral("action"), RtcWakeController::actionLabel(action));

    QJsonDocument doc(payload);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.write("\n");
}
