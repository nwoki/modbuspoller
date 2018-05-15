#ifndef MODBUSPOLLER_POLLER_P_H
#define MODBUSPOLLER_POLLER_P_H

#include <modbus.h>

#include "poller.h"
#include "poller_global.h"

#include <QtCore/QQueue>
#include <QtCore/QTimer>

#include <QtSerialBus/QModbusDataUnit>


class QModbusClient;


namespace ModbusPoller {

class MODBUSPOLLERSHARED_EXPORT PollerPrivate
{
public:
    PollerPrivate(Poller::Backend backend)
        : pollTimer(new QTimer)
        , modbusClient(nullptr)
        , libModbusClient(nullptr)
        , connectionState(Poller::UNCONNECTED)
        , state(Poller::IDLE)
        , backend(backend)
    {}

    ~PollerPrivate()
    {
        if (pollTimer->isActive()) {
            pollTimer->stop();
        }

        delete pollTimer;
    }

    QTimer *pollTimer;

    // the modbus client objs
    QModbusClient *modbusClient;
    modbus_t *libModbusClient;

    // read / write queues
    QQueue<QModbusDataUnit> readQueue;
    QQueue<QModbusDataUnit> writeQueue;

    QModbusDataUnit defaultPollCommand;

    // poller gen data
    Poller::ConnectionState connectionState;
    Poller::State state;
    Poller::Backend backend;
};

}   // ModbusPoller


#endif  // MODBUSPOLLER_POLLER_P_H
