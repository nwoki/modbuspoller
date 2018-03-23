#ifndef MODBUSPOLLER_POLLER_P_H
#define MODBUSPOLLER_POLLER_P_H

#include <QtCore/QTimer>


namespace ModbusPoller {

class PollerPrivate
{
public:
    PollerPrivate()
        : pollTimer(new QTimer)
    {}

    ~PollerPrivate()
    {
        if (pollTimer->isActive()) {
            pollTimer->stop();
        }

        delete pollTimer;
    }


    QTimer *pollTimer;
};

}   // ModbusPoller


#endif  // MODBUSPOLLER_POLLER_P_H
