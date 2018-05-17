#include "actionthread.h"

#include <QtCore/QDebug>

#include <QtSerialBus/QModbusDataUnit>

namespace ModbusPoller {

ActionThread::ActionThread(const QSharedPointer<QQueue<QModbusDataUnit>> &actionQueue, QObject *parent)
    : QThread(parent)
    , m_actionQueue(actionQueue)
    , m_modbusConn(nullptr)
{
}

ActionThread::~ActionThread()
{
    m_actionQueue = nullptr;
}

QSharedPointer<QQueue<QModbusDataUnit>> ActionThread::actionQueue() const
{
    return m_actionQueue;
}

modbus_t *ActionThread::modbusConnection() const
{
    return m_modbusConn;
}

void ActionThread::setModbusConnection(modbus_t *modbusConnection)
{
    m_modbusConn = modbusConnection;
}

}   // ModbusPoller
