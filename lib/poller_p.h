#ifndef MODBUSPOLLER_POLLER_P_H
#define MODBUSPOLLER_POLLER_P_H

#include <QtCore/QQueue>
#include <QtCore/QTimer>

#include <QtSerialBus/QModbusDataUnit>


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
    QQueue<QModbusDataUnit> readQueue;
    QQueue<QModbusDataUnit> writeQueue;

    QModbusDataUnit defaultPollCommand;
};

}   // ModbusPoller


#endif  // MODBUSPOLLER_POLLER_P_H
