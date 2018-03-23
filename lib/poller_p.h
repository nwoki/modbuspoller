#ifndef MODBUSPOLLER_POLLER_P_H
#define MODBUSPOLLER_POLLER_P_H

#include <QtCore/QTimer>

class QModbusClient;


namespace ModbusPoller {

class PollerPrivate
{
public:
    PollerPrivate()
        : pollTimer(new QTimer)
        , modbusClient(nullptr)
    {}

    ~PollerPrivate()
    {
        if (pollTimer->isActive()) {
            pollTimer->stop();
        }

        delete pollTimer;
    }

    QTimer *pollTimer;
    QModbusClient *modbusClient;
};

}   // ModbusPoller


#endif  // MODBUSPOLLER_POLLER_P_H
