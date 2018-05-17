#ifndef MODBUSPOLLER_READACTIONTHREAD_H
#define MODBUSPOLLER_READACTIONTHREAD_H

#include "actionthread.h"

#include <QtSerialBus/QModbusDataUnit>

namespace ModbusPoller {

/**
 * @brief The ReadActionThread class
 * @author Francesco Nwokeka <francesco.nwokeka@gmail.com>
 *
 * The Read thread to use for libmodbus backend.
 */
class ReadActionThread : public ActionThread
{
    Q_OBJECT

public:
    ReadActionThread(const QSharedPointer<QQueue<QModbusDataUnit>> &readQueue, QObject *parent = nullptr);
    ~ReadActionThread();

Q_SIGNALS:
    void modbusResponseReceived(const QModbusDataUnit &dataUnit);

protected:
    void run() override final;

private:

};

}

#endif // MODBUSPOLLER_READACTIONTHREAD_H
