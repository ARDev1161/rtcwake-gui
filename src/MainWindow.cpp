#include "MainWindow.h"

#include "AnalogClockWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCloseEvent>
#include <QDateEdit>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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
#include <QSlider>
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

const QString kDefaultSoundPath = QStringLiteral(":/rtcwake/sounds/chime.wav");
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

    m_soundEnabled = new QCheckBox(tr("Play sound alert"), warningBox);
    m_soundFile = new QLineEdit(kDefaultSoundPath, warningBox);
    m_soundFile->setPlaceholderText(kDefaultSoundPath);
    m_soundBrowse = new QPushButton(tr("Browse..."), warningBox);
    m_soundReset = new QPushButton(tr("Use bundled tone"), warningBox);
    auto *soundPathWidget = new QWidget(warningBox);
    auto *soundPathLayout = new QHBoxLayout(soundPathWidget);
    soundPathLayout->setContentsMargins(0, 0, 0, 0);
    soundPathLayout->addWidget(m_soundFile);
    soundPathLayout->addWidget(m_soundBrowse);
    soundPathLayout->addWidget(m_soundReset);

    m_soundVolume = new QSlider(Qt::Horizontal, warningBox);
    m_soundVolume->setRange(0, 100);
    m_soundVolume->setValue(70);
    m_soundVolumeLabel = new QLabel(tr("70%"), warningBox);
    auto *volumeWidget = new QWidget(warningBox);
    auto *volumeLayout = new QHBoxLayout(volumeWidget);
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    volumeLayout->addWidget(m_soundVolume, 1);
    volumeLayout->addWidget(m_soundVolumeLabel);

    m_themeCombo = new QComboBox(warningBox);
    m_themeCombo->addItem(tr("Crimson (high alert)"), QStringLiteral("crimson"));
    m_themeCombo->addItem(tr("Amber (warning)"), QStringLiteral("amber"));
    m_themeCombo->addItem(tr("Emerald (calm)"), QStringLiteral("emerald"));
    m_themeCombo->addItem(tr("Slate (dark)"), QStringLiteral("slate"));

    m_fullscreenBanner = new QCheckBox(tr("Show fullscreen"), warningBox);
    m_bannerWidth = new QSpinBox(warningBox);
    m_bannerWidth->setRange(320, 3840);
    m_bannerWidth->setValue(640);
    m_bannerWidth->setSuffix(tr(" px"));
    m_bannerHeight = new QSpinBox(warningBox);
    m_bannerHeight->setRange(200, 2160);
    m_bannerHeight->setValue(360);
    m_bannerHeight->setSuffix(tr(" px"));

    warningLayout->addWidget(m_warningEnabled, 0, 0, 1, 2);
    warningLayout->addWidget(new QLabel(tr("Message:"), warningBox), 1, 0);
    warningLayout->addWidget(m_warningMessage, 1, 1);
    warningLayout->addWidget(new QLabel(tr("Countdown:"), warningBox), 2, 0);
    warningLayout->addWidget(m_warningCountdown, 2, 1);
    warningLayout->addWidget(new QLabel(tr("Snooze interval:"), warningBox), 3, 0);
    warningLayout->addWidget(m_warningSnooze, 3, 1);
    warningLayout->addWidget(m_soundEnabled, 4, 0, 1, 2);
    warningLayout->addWidget(new QLabel(tr("Sound file:"), warningBox), 5, 0);
    warningLayout->addWidget(soundPathWidget, 5, 1);
    warningLayout->addWidget(new QLabel(tr("Volume:"), warningBox), 6, 0);
    warningLayout->addWidget(volumeWidget, 6, 1);
    warningLayout->addWidget(new QLabel(tr("Color theme:"), warningBox), 7, 0);
    warningLayout->addWidget(m_themeCombo, 7, 1);
    warningLayout->addWidget(m_fullscreenBanner, 8, 0, 1, 2);
    warningLayout->addWidget(new QLabel(tr("Dialog width:"), warningBox), 9, 0);
    warningLayout->addWidget(m_bannerWidth, 9, 1);
    warningLayout->addWidget(new QLabel(tr("Dialog height:"), warningBox), 10, 0);
    warningLayout->addWidget(m_bannerHeight, 10, 1);

    layout->addWidget(warningBox);
    layout->addStretch();
    updateSoundControls();
    updateBannerSizeControls();

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

    if (m_soundEnabled) {
        connect(m_soundEnabled, &QCheckBox::toggled, this, [this](bool) { updateSoundControls(); });
    }
    if (m_soundBrowse) {
        connect(m_soundBrowse, &QPushButton::clicked, this, [this]() {
            const QString start = m_soundFile && !m_soundFile->text().isEmpty() ? QFileInfo(m_soundFile->text()).absolutePath()
                                                                                : QDir::homePath();
            const QString filter = tr("Audio files (*.wav *.mp3 *.ogg *.flac);;All files (*)");
            const QString selected = QFileDialog::getOpenFileName(this, tr("Select sound file"), start, filter);
            if (!selected.isEmpty()) {
                m_soundFile->setText(selected);
            }
        });
    }
    if (m_soundReset) {
        connect(m_soundReset, &QPushButton::clicked, this, [this]() {
            if (m_soundFile) {
                m_soundFile->setText(kDefaultSoundPath);
            }
        });
    }
    if (m_soundVolume) {
        connect(m_soundVolume, &QSlider::valueChanged, this, [this](int value) {
            if (m_soundVolumeLabel) {
                m_soundVolumeLabel->setText(tr("%1%").arg(value));
            }
        });
    }
    if (m_fullscreenBanner) {
        connect(m_fullscreenBanner, &QCheckBox::toggled, this, [this](bool) { updateBannerSizeControls(); });
    }
}

void MainWindow::updateSoundControls() {
    const bool enabled = m_soundEnabled && m_soundEnabled->isChecked();
    const auto toggle = [enabled](QWidget *widget) {
        if (widget) {
            widget->setEnabled(enabled);
        }
    };
    toggle(m_soundFile);
    toggle(m_soundBrowse);
    toggle(m_soundReset);
    if (m_soundVolume) {
        m_soundVolume->setEnabled(enabled);
    }
    if (m_soundVolumeLabel) {
        m_soundVolumeLabel->setEnabled(enabled);
        const int value = m_soundVolume ? m_soundVolume->value() : 0;
        m_soundVolumeLabel->setText(tr("%1%").arg(value));
    }
}

void MainWindow::updateBannerSizeControls() {
    const bool enableSize = !(m_fullscreenBanner && m_fullscreenBanner->isChecked());
    if (m_bannerWidth) {
        m_bannerWidth->setEnabled(enableSize);
    }
    if (m_bannerHeight) {
        m_bannerHeight->setEnabled(enableSize);
    }
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
    if (m_soundEnabled) {
        m_soundEnabled->setChecked(m_config.warning.soundEnabled);
    }
    if (m_soundFile) {
        m_soundFile->setText(m_config.warning.soundFile);
    }
    if (m_soundVolume) {
        m_soundVolume->setValue(m_config.warning.soundVolume);
    }
    if (m_themeCombo) {
        const int idx = m_themeCombo->findData(m_config.warning.theme);
        if (idx >= 0) {
            m_themeCombo->setCurrentIndex(idx);
        }
    }
    if (m_fullscreenBanner) {
        m_fullscreenBanner->setChecked(m_config.warning.fullscreen);
    }
    if (m_bannerWidth) {
        m_bannerWidth->setValue(m_config.warning.width);
    }
    if (m_bannerHeight) {
        m_bannerHeight->setValue(m_config.warning.height);
    }
    updateSoundControls();
    updateBannerSizeControls();

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
    m_config.actionId = static_cast<int>(currentAction());

    m_config.warning.enabled = m_warningEnabled->isChecked();
    m_config.warning.message = m_warningMessage->text();
    m_config.warning.countdownSeconds = m_warningCountdown->value();
    m_config.warning.snoozeMinutes = m_warningSnooze->value();
    m_config.warning.soundEnabled = m_soundEnabled && m_soundEnabled->isChecked();
    QString soundPath = m_soundFile ? m_soundFile->text().trimmed() : QString();
    if (soundPath.isEmpty()) {
        soundPath = kDefaultSoundPath;
    }
    m_config.warning.soundFile = soundPath;
    if (m_soundVolume) {
        m_config.warning.soundVolume = m_soundVolume->value();
    }
    if (m_themeCombo) {
        const QString theme = m_themeCombo->currentData().toString();
        if (!theme.isEmpty()) {
            m_config.warning.theme = theme;
        }
    }
    if (m_fullscreenBanner) {
        m_config.warning.fullscreen = m_fullscreenBanner->isChecked();
    }
    if (m_bannerWidth) {
        m_config.warning.width = m_bannerWidth->value();
    }
    if (m_bannerHeight) {
        m_config.warning.height = m_bannerHeight->value();
    }

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
