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

    if (actionQueue().data()->count() > 0) {
        // check we have a valid connection pointer otherwise clear out the queue and return
        if (modbusConnection() == nullptr) {
            clearActionQueue();
            return;
        }

        // extract and prepare
        QModbusDataUnit du = actionQueue().data()->dequeue();
        int valuesCount = du.values().count();
        uint16_t *dest16 = new uint16_t[valuesCount]{};

        // don't forget to tell modbus from which slave to read from
        modbus_set_slave(modbusConnection(), 1);

        bool encountedError = false;

        switch (du.registerType()) {
            case QModbusDataUnit::HoldingRegisters:
                if (modbus_read_registers(modbusConnection(), du.startAddress(), valuesCount, dest16) == -1) {
                    qDebug("[ReadActionThread::run] ERROR READING FROM MODBUS");
                    Q_EMIT modbusReadError(QString("[ReadActionThread::run] %1").arg(modbus_strerror(errno)));
                    encountedError = true;
                }
            break;
            case QModbusDataUnit::InputRegisters:
                if (modbus_read_input_registers(modbusConnection(), du.startAddress(), valuesCount, dest16) == -1) {
                    qDebug("[ReadActionThread::run] ERROR READING FROM MODBUS");
                    Q_EMIT modbusReadError(QString("[ReadActionThread::run] %1").arg(modbus_strerror(errno)));
                    encountedError = true;
                }
            break;

            // TODO : other register type cases. Currently unhandled
            case QModbusDataUnit::Invalid:
            case QModbusDataUnit::DiscreteInputs:
            case QModbusDataUnit::Coils:
            default:
                // TODO
            break;
        }

        // create the response object. Don't send the data if we have an error. It will result in a misread (all values set to 0)
        if (!encountedError) {
            QVector<quint16> readValues;
            for (int i = 0; i < valuesCount; ++i) {
                readValues.push_back(dest16[i]);
            }

            Q_EMIT modbusResponseReceived(QModbusDataUnit(du.registerType(), du.startAddress(), readValues));
        }
    }
}

}
