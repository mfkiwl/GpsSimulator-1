#ifndef QGPS_RECEIVER_HPP
#define QGPS_RECEIVER_HPP

#include <QStatusButton.hpp>
#include <QLabel>

#include <gps.h>
#include <thread>

class QGpsReceiver : public QStatusButton
{
Q_OBJECT

public:
    explicit QGpsReceiver (QWidget* parent = nullptr);

    ~QGpsReceiver () override;

    void startReceiver ();

private slots:

    void setErrorText (const QString& error, Status status);

    void setMainText (const QString& text);

signals:

    void errorEncountered (const QString& error, Status status);

    void fixAcquired (const QString& text);

private:
    std::atomic<bool> threadRunning;

    std::thread gpsQueryThread;

    QLabel* mainText;

    QLabel* errorText;

    struct gps_data_t gpsData;

    void gpsquery_task ();
};

#endif //QGPS_RECEIVER_HPP