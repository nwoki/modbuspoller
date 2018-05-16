#include "modbus.h"
#include "readactionthread.h"

#include <QtCore/QDebug>
#include <QtCore/QQueue>



namespace ModbusPoller {

ReadActionThread::ReadActionThread(const QSharedPointer<QQueue<QModbusDataUnit>> &readQueue, QObject *parent)
    : ActionThread(readQueue, parent)
{
}

ReadActionThread::~ReadActionThread()
{
    qDebug("[ReadActionThread::~ReadActionThread]");
}

void ReadActionThread::run()
{
    qDebug("[ReadActionThread::run]");

    // first check that we're not running with an invalid modbus connection
    // invalid connection struct
    if (modbusConnection() == nullptr) {
        return;
    }

    while (actionQueue().data()->count() > 0) {
        // extract and prepare
        QModbusDataUnit du = actionQueue().data()->dequeue();
        int valuesCount = du.values().count();
        uint16_t *dest16 = new uint16_t[valuesCount]{};

        // don't forget to tell modbus from which slave to read from
        modbus_set_slave(modbusConnection(), 1);

        qDebug() << "START ADDR: " << du.startAddress();
        qDebug() << "VALUES: " << du.values();
        qDebug() << "VALUE COUNT: " << du.valueCount();

        switch (du.registerType()) {
            case QModbusDataUnit::HoldingRegisters:
                if (modbus_read_registers(modbusConnection(), du.startAddress(), valuesCount, dest16) == -1) {
                    qDebug("[ReadActionThread::run] ERROR READING FROM MODBUS");
                }
            break;
            case QModbusDataUnit::InputRegisters:
                if (modbus_read_input_registers(modbusConnection(), du.startAddress(), valuesCount, dest16) == -1) {
                    qDebug("[ReadActionThread::run] ERROR READING FROM MODBUS");
                }
            break;

            // TODO : other register type cases
        }

        // create the response object
        QVector<quint16> readValues;
        for (int i = 0; i < valuesCount; ++i) {
//            qDebug() << "FROM PLC : " << dest16[i];
            readValues.push_back(dest16[i]);
        }

        Q_EMIT modbusResponseReceived(QModbusDataUnit(du.registerType(), du.startAddress(), readValues));
    }
}

}
