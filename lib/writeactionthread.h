#ifndef MODBUSPOLLER_WRITEACTIONTHREAD_H
#define MODBUSPOLLER_WRITEACTIONTHREAD_H


#include "actionthread.h"

namespace ModbusPoller {

/**
 * @brief The WriteActionThread class
 * @author Francesco Nwokeka <francesco.nwokeka@gmail.com>
 *
 * The Write thread to use for libmodbus backend.
 */
class WriteActionThread : public ActionThread
{
    Q_OBJECT

public:
    WriteActionThread(const QSharedPointer<QQueue<QModbusDataUnit>> &writeQueue, QObject *parent = nullptr);
    ~WriteActionThread();

protected:
    void run() override final;

private:

};

}

#endif // MODBUSPOLLER_WRITEACTIONTHREAD_H
