#pragma once

#include <QTime>
#include <QWidget>

/**
 * @brief Simple analog clock widget used for visualising the selected wake time.
 */
class AnalogClockWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnalogClockWidget(QWidget *parent = nullptr);

    /** Update the displayed time. */
    void setTime(const QTime &time);
    QTime time() const { return m_time; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QTime m_time;
};
