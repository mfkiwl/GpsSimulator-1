
#include <QGpsReceiver.hpp>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>

QGpsReceiver::QGpsReceiver (QWidget* parent) : QStatusButton (parent), threadRunning (false)
{
    auto* vBoxLayout = new QVBoxLayout (this);

    QPalette pal = palette ();
    pal.setBrush (QPalette::All, QPalette::Text, Qt::black);
    setPalette (pal);

    mainText = new QLabel ("No Fix");
    mainText->setFont (QFont ("Ubuntu Mono", 18, QFont::Bold));
    mainText->setAlignment (Qt::AlignCenter);
    vBoxLayout->addWidget (mainText);

    errorText = new QLabel ("");
    errorText->setFont (QFont ("Ubuntu Mono", 14, QFont::Medium));
    errorText->setAlignment (Qt::AlignCenter);
    errorText->hide ();
    vBoxLayout->addWidget (errorText);

    qRegisterMetaType<Status> ();

    connect (this, &QGpsReceiver::fixAcquired, this, &QGpsReceiver::setMainText);
    connect (this, &QGpsReceiver::errorEncountered, this, &QGpsReceiver::setErrorText);
}

QGpsReceiver::~QGpsReceiver ()
{
    if (threadRunning)
    {
        threadRunning = false;
        gpsQueryThread.join ();
    }

    delete mainText;
    delete errorText;
}

void QGpsReceiver::startReceiver ()
{
    threadRunning = true;
    gpsQueryThread = std::thread (&QGpsReceiver::gpsquery_task, this);
}

void QGpsReceiver::setMainText (const QString& text)
{
    mainText->setText (text);
}

void QGpsReceiver::setErrorText (const QString& error, Status status)
{
    errorText->setText (error);
    setStatus (status);
    switch (status)
    {
        case OFF:
        case OK:
            errorText->hide ();
            break;
        case WARNING:
        case ERROR:
            errorText->show ();
            break;
    }
}

void QGpsReceiver::gpsquery_task ()
{
    int ret = gps_open ("localhost", "2947", &gpsData);
    if (ret)
    {
        emit errorEncountered (QString::fromStdString (gps_errstr (ret)), ERROR);
        return;
    }
    errorText->hide ();

    gps_stream (&gpsData, WATCH_ENABLE | WATCH_JSON, nullptr);

    while (threadRunning)
    {
        // Wait for 5 seconds for data to appear.
        if (gps_waiting (&gpsData, 5000000))
        {
            if (gps_read (&gpsData, nullptr, 0) != -1)
            {
                if (gpsData.fix.status == STATUS_FIX &&
                    (gpsData.fix.mode == MODE_2D || gpsData.fix.mode == MODE_3D) &&
                    !std::isnan (gpsData.fix.latitude) &&
                    !std::isnan (gpsData.fix.longitude))
                {
                    if (gpsData.fix.mode == MODE_2D)
                    {
                        std::string display = "2D Fix\n" + std::to_string (gpsData.fix.latitude) + "," +
                                              std::to_string (gpsData.fix.longitude);
                        emit fixAcquired (QString::fromStdString (display));
                    } else
                    {
                        std::string display = "3D Fix\n" + std::to_string (gpsData.fix.latitude) + "," +
                                              std::to_string (gpsData.fix.longitude) + "," +
                                              std::to_string (gpsData.fix.altitude);
                        emit fixAcquired (QString::fromStdString (display));
                    }
                }
            }
        }
    }

    gps_stream (&gpsData, WATCH_DISABLE, nullptr);
    gps_close (&gpsData);
}