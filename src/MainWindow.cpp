#include "MainWindow.h"

#include "AnalogClockWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateEdit>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <algorithm>

namespace {
QString formatDateTime(const QDateTime &dt) {
    QLocale locale;
    return locale.toString(dt, QLocale::LongFormat);
}

QString currentUser() {
    const QByteArray user = qgetenv("USER");
    if (!user.isEmpty()) {
        return QString::fromLocal8Bit(user);
    }
    return QStringLiteral("unknown");
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    buildUi();
    connectSignals();
    loadSettings();

    setWindowIcon(QIcon(QStringLiteral(":/rtcwake/icons/clock.svg")));
    setWindowTitle(tr("rtcwake planner"));
}

void MainWindow::buildUi() {
    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildSingleTab(), tr("Single schedule"));
    tabs->addTab(buildWeeklyTab(), tr("Weekly schedule"));
    tabs->addTab(buildSettingsTab(), tr("Settings"));

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);

    m_nextSummary = new QLabel(tr("Configuration not saved yet."), this);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(tabs);
    layout->addWidget(m_nextSummary);
    layout->addWidget(new QLabel(tr("Activity log:"), this));
    layout->addWidget(m_log, 1);
    setCentralWidget(central);
    resize(960, 720);
}

QWidget *MainWindow::buildSingleTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    auto *shutdownBox = new QGroupBox(tr("Shutdown time"), tab);
    auto *shutdownLayout = new QGridLayout(shutdownBox);
    m_shutdownDateEdit = new QDateEdit(QDate::currentDate(), shutdownBox);
    m_shutdownDateEdit->setCalendarPopup(true);
    m_shutdownTimeEdit = new QTimeEdit(QTime::currentTime(), shutdownBox);
    m_shutdownTimeEdit->setDisplayFormat(QStringLiteral("HH:mm"));

    shutdownLayout->addWidget(new QLabel(tr("Date:"), shutdownBox), 0, 0);
    shutdownLayout->addWidget(m_shutdownDateEdit, 0, 1);
    shutdownLayout->addWidget(new QLabel(tr("Time:"), shutdownBox), 1, 0);
    shutdownLayout->addWidget(m_shutdownTimeEdit, 1, 1);
    layout->addWidget(shutdownBox);

    auto *wakeBox = new QGroupBox(tr("Wake time"), tab);
    auto *wakeLayout = new QGridLayout(wakeBox);
    m_dateEdit = new QDateEdit(QDate::currentDate(), wakeBox);
    m_dateEdit->setCalendarPopup(true);
    m_timeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600), wakeBox);
    m_timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    m_timeEdit->setMinimumWidth(120);

    wakeLayout->addWidget(new QLabel(tr("Date:"), wakeBox), 0, 0);
    wakeLayout->addWidget(m_dateEdit, 0, 1);
    wakeLayout->addWidget(new QLabel(tr("Time:"), wakeBox), 1, 0);
    wakeLayout->addWidget(m_timeEdit, 1, 1);

    m_clockWidget = new AnalogClockWidget(wakeBox);
    wakeLayout->addWidget(m_clockWidget, 0, 2, 2, 1);

    layout->addWidget(wakeBox);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    auto *saveButton = new QPushButton(tr("Save single schedule"), tab);
    buttonRow->addWidget(saveButton);
    layout->addLayout(buttonRow);

    connect(saveButton, &QPushButton::clicked, this, &MainWindow::scheduleSingleWake);

    return tab;
}

QWidget *MainWindow::buildWeeklyTab() {
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);

    m_scheduleTable = new QTableWidget(7, 4, tab);
    m_scheduleTable->setHorizontalHeaderLabels({tr("Enabled"), tr("Day"), tr("Shutdown"), tr("Wake")});
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_scheduleTable->verticalHeader()->setVisible(false);

    const QList<Qt::DayOfWeek> days {
        Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday,
        Qt::Friday, Qt::Saturday, Qt::Sunday
    };

    QLocale locale;

    for (int row = 0; row < days.size(); ++row) {
        auto day = days.at(row);
        auto *enabled = new QCheckBox(tab);
        auto *shutdownEdit = new QTimeEdit(QTime(23, 0), tab);
        shutdownEdit->setDisplayFormat(QStringLiteral("HH:mm"));
        auto *wakeEdit = new QTimeEdit(QTime(7, 30), tab);
        wakeEdit->setDisplayFormat(QStringLiteral("HH:mm"));

        auto *dayItem = new QTableWidgetItem(locale.dayName(day));
        dayItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_scheduleTable->setCellWidget(row, 0, enabled);
        m_scheduleTable->setItem(row, 1, dayItem);
        m_scheduleTable->setCellWidget(row, 2, shutdownEdit);
        m_scheduleTable->setCellWidget(row, 3, wakeEdit);

        m_weeklyRows.push_back({day, enabled, shutdownEdit, wakeEdit});
    }

    layout->addWidget(m_scheduleTable);

    auto *hint = new QLabel(tr("Enable the days you want to automate. Shutdown happens at the configured time, and the daemon programs rtcwake for the wake time."), tab);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *button = new QPushButton(tr("Save weekly schedule"), tab);
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
        if (m_clockWidget) {
            m_clockWidget->setTime(time);
        }
    });
}

void MainWindow::loadSettings() {
    m_config = m_configRepo.load();
    applyConfigToUi();
    m_nextSummary->setText(tr("Config loaded for %1").arg(currentUser()));
}

void MainWindow::applyConfigToUi() {
    m_shutdownDateEdit->setDate(m_config.singleShutdownDate);
    m_shutdownTimeEdit->setTime(m_config.singleShutdownTime);
    m_dateEdit->setDate(m_config.singleWakeDate);
    m_timeEdit->setTime(m_config.singleWakeTime);
    if (m_clockWidget) {
        m_clockWidget->setTime(m_timeEdit->time());
    }

    if (auto *button = m_actionGroup->button(m_config.actionId)) {
        button->setChecked(true);
    }

    m_warningEnabled->setChecked(m_config.warning.enabled);
    m_warningMessage->setText(m_config.warning.message);
    m_warningCountdown->setValue(m_config.warning.countdownSeconds);
    m_warningSnooze->setValue(m_config.warning.snoozeMinutes);

    for (auto &row : m_weeklyRows) {
        if (auto *entry = weeklyConfig(row.day)) {
            row.enabled->setChecked(entry->enabled);
            row.shutdownEdit->setTime(entry->shutdownTime);
            row.wakeEdit->setTime(entry->wakeTime);
        } else {
            row.enabled->setChecked(false);
        }
    }
}

void MainWindow::collectUiIntoConfig() {
    m_config.singleShutdownDate = m_shutdownDateEdit->date();
    m_config.singleShutdownTime = m_shutdownTimeEdit->time();
    m_config.singleWakeDate = m_dateEdit->date();
    m_config.singleWakeTime = m_timeEdit->time();
    m_config.actionId = currentAction() == PowerAction::None ? static_cast<int>(PowerAction::SuspendToRam)
                                                             : static_cast<int>(currentAction());

    m_config.warning.enabled = m_warningEnabled->isChecked();
    m_config.warning.message = m_warningMessage->text();
    m_config.warning.countdownSeconds = m_warningCountdown->value();
    m_config.warning.snoozeMinutes = m_warningSnooze->value();

    for (const auto &row : m_weeklyRows) {
        if (auto *entry = weeklyConfig(row.day)) {
            entry->enabled = row.enabled->isChecked();
            entry->shutdownTime = row.shutdownEdit->time();
            entry->wakeTime = row.wakeEdit->time();
        }
    }

    m_config.session.user = currentUser();
    m_config.session.display = qEnvironmentVariable("DISPLAY");
    m_config.session.xdgRuntimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    m_config.session.dbusAddress = qEnvironmentVariable("DBUS_SESSION_BUS_ADDRESS");
}

void MainWindow::saveSettings(const QString &reason) {
    collectUiIntoConfig();
    if (!m_configRepo.save(m_config)) {
        QMessageBox::critical(this, tr("Save error"), tr("Failed to write configuration file."));
        return;
    }

    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_nextSummary->setText(tr("Saved at %1").arg(stamp));
    appendLog(reason.isEmpty() ? tr("Configuration updated.") : reason);
}

void MainWindow::scheduleSingleWake() {
    const QDateTime shutdown(m_shutdownDateEdit->date(), m_shutdownTimeEdit->time());
    const QDateTime wake(m_dateEdit->date(), m_timeEdit->time());
    if (!shutdown.isValid() || !wake.isValid()) {
        QMessageBox::warning(this, tr("Invalid time"), tr("Please select valid shutdown and wake times."));
        return;
    }
    if (shutdown >= wake) {
        QMessageBox::warning(this, tr("Invalid ordering"), tr("Shutdown time must be before wake time."));
        return;
    }
    saveSettings(tr("Single schedule saved."));
}

void MainWindow::scheduleNextFromWeekly() {
    bool anyEnabled = false;
    for (const auto &row : m_weeklyRows) {
        if (row.enabled->isChecked()) {
            anyEnabled = true;
            break;
        }
    }
    if (!anyEnabled) {
        QMessageBox::information(this, tr("No days enabled"), tr("Enable at least one weekday."));
        return;
    }
    saveSettings(tr("Weekly schedule saved."));
}

PowerAction MainWindow::currentAction() const {
    const int id = m_actionGroup->checkedId();
    if (id == -1) {
        return PowerAction::None;
    }
    return static_cast<PowerAction>(id);
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

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings(QString());
    QMainWindow::closeEvent(event);
}
