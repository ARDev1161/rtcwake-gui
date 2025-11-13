#include "MainWindow.h"

#include "AnalogClockWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    buildUi();
}

void MainWindow::buildUi() {
    auto *tabWidget = new QTabWidget(this);

    auto *singleTab = new QWidget(this);
    auto *singleLayout = new QVBoxLayout(singleTab);

    auto *timeLabel = new QLabel(tr("Select wake time:"), singleTab);
    auto *timeEdit = new QTimeEdit(QTime::currentTime(), singleTab);
    timeEdit->setDisplayFormat("HH:mm");

    m_clockWidget = new AnalogClockWidget(singleTab);
    m_clockWidget->setTime(timeEdit->time());

    connect(timeEdit, &QTimeEdit::timeChanged, m_clockWidget, &AnalogClockWidget::setTime);

    singleLayout->addWidget(timeLabel);
    singleLayout->addWidget(timeEdit);
    singleLayout->addWidget(m_clockWidget, 1);

    auto *scheduleTab = new QWidget(this);
    auto *scheduleLayout = new QVBoxLayout(scheduleTab);
    scheduleLayout->addWidget(new QLabel(tr("Weekly schedule configuration will appear here."), scheduleTab));

    tabWidget->addTab(singleTab, tr("Single wake"));
    tabWidget->addTab(scheduleTab, tr("Weekly schedule"));

    setCentralWidget(tabWidget);
    setWindowTitle(tr("rtcwake GUI"));
    resize(640, 480);
}
