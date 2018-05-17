#include "writeactionthread.h"

#include <QtCore/QDebug>
#include <QtCore/QQueue>
#include <QtSerialBus/QModbusDataUnit>


namespace ModbusPoller {

WriteActionThread::WriteActionThread(const QSharedPointer<QQueue<QModbusDataUnit>> &writeQueue, QObject *parent)
    : ActionThread(writeQueue, parent)
{
}

WriteActionThread::~WriteActionThread()
{
    qDebug("[WriteActionThread::~WriteActionThread]");
}

void WriteActionThread::run()
{
    qDebug("[WriteActionThread::run]");

    // first check that we're not running with an invalid modbus connection
    // invalid connection struct
    if (modbusConnection() == nullptr) {
        return;
    }

    if (actionQueue().data()->count() > 0) {
        // extract and prepare
        QModbusDataUnit du = actionQueue().data()->dequeue();
        int valuesCount = du.values().count();
        uint16_t *dest16 = new uint16_t[valuesCount]{};

        // we need to populate the dest16 array with values we want to write
        for (int i = 0; i < valuesCount; ++i) {
            dest16[i] = du.value(i);
        }

        // don't forget to tell modbus from which slave to read from
        modbus_set_slave(modbusConnection(), 1);

        switch (du.registerType()) {
            case QModbusDataUnit::HoldingRegisters:
                if (modbus_write_registers(modbusConnection(), du.startAddress(), valuesCount, dest16) == -1) {
                    qDebug("[WriteActionThread::run] error writing to registers!");
                }
            break;
        }
    }
}

}   // ModbusPoller
