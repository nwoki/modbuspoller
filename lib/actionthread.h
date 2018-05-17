#ifndef MODBUSPOLLER_ACTIONTHREAD_H
#define MODBUSPOLLER_ACTIONTHREAD_H

#include "modbus.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QThread>


class QModbusDataUnit;


namespace ModbusPoller {

/**
 * @brief The ActionThread class
 * @author Francesco Nwokeka <francesco.nwokeka@gmail.com>
 *
 * Base class for the action thread. It takes in a QQueue of QModbusDataUnits to act upon. For reading purposes
 * use the ReadActionThread class. For writing, the WriteActionThread class
 *
 * NOTE: REMEMBER (yeah, this sucks) that on every modbus_t connection change/renewal, to set it in this action
 * thread class!
 */
class ActionThread : public QThread
{
    Q_OBJECT

public:
    ActionThread(const QSharedPointer<QQueue<QModbusDataUnit>> &actionQueue, QObject *parent = nullptr);
    ~ActionThread();

    QSharedPointer<QQueue<QModbusDataUnit>> actionQueue() const;
    virtual void run() = 0;
    void setModbusConnection(modbus_t *modbusConnection);

protected:
    modbus_t *modbusConnection() const;

private:
    QSharedPointer<QQueue<QModbusDataUnit>> m_actionQueue;
    modbus_t *m_modbusConn;
};

}

#endif // MODBUSPOLLER_ACTIONTHREAD_H
