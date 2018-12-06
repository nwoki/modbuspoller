#include "poller.h"
#include "poller_p.h"
#include "readactionthread.h"
#include "writeactionthread.h"

#include <QtCore/QDebug>

#include <QtSerialBus/QModbusClient>
#include <QtSerialBus/QModbusRtuSerialMaster>
#include <QtSerialBus/QModbusTcpClient>

#include <modbus-rtu.h>

// NOTE (A): needed for the connection from the libmodbus action threads. No needed with other qt classes. Investigate why
Q_DECLARE_METATYPE(QModbusDataUnit)

namespace ModbusPoller {

Poller::Poller(Backend backend, const QModbusDataUnit &defaultCommand, quint16 pollingInterval, QObject *parent)
    : QObject(parent)
    , d(new PollerPrivate(backend))
{
    // NOTE (A): same here.
    qRegisterMetaType<QModbusDataUnit>();

    d->defaultPollCommand = defaultCommand;
    d->pollTimer->setInterval(pollingInterval);
    d->pollTimer->setSingleShot(true);

    connect(d->pollTimer, &QTimer::timeout, this, &Poller::onPollTimeout);

    // notify starting state
    Q_EMIT connectionStateChanged(d->connectionState);
    Q_EMIT stateChanged(d->state);

    if (d->backend == LibModbusBackend) {
        d->readActionThread = new ReadActionThread(d->readQueue, this);
        d->writeActionThread = new WriteActionThread(d->writeQueue, this);

        connect(d->readActionThread, &ReadActionThread::modbusResponseReceived, this, &Poller::onLibModbusReplyFinished);
        connect(d->readActionThread, &ReadActionThread::modbusReadError, this, &Poller::onLibModbusReadError);

        // it's sufficient for us to just check the thread runtime for determining when the write procedure finishes as the write
        // to modbus doesn't have to return any data
        connect(d->writeActionThread, &WriteActionThread::finished, this, &Poller::onLibmodbusWriteFinished);
    }
}

Poller::~Poller()
{
    delete d;
}

void Poller::connectDevice(uint32_t responseTimeoutSec, uint32_t responseTimeoutUSec)
{
    if (d->backend == QtModbusBackend) {
        if (!d->modbusClient) {
            qDebug("[Poller::connectDevice] modbus client not configured!");
            Q_EMIT connectionError("modbus client not configured");
            return;
        }

        if (!d->modbusClient->connectDevice()) {
            qDebug() << "[Poller::connectDevice] CONNECTION ERROR: " << d->modbusClient->errorString();
            Q_EMIT connectionError(QString("[Poller::connectDevice] Error connecting to device: %1").arg(d->modbusClient->errorString()));
        }

        qDebug("All is well. Start polling");
    } else {
        if (!d->libModbusClient) {
            qDebug("[Poller::connectDevice libmodbus] modbus client not configured!");
            Q_EMIT connectionError("modbus client not configured");
            return;
        }

        if(modbus_connect(d->libModbusClient) == -1) {
            qDebug() << "[Poller::connectDevice] Could not connect serial port!";
            Q_EMIT connectionError(QString("[Poller::connectDevice] Error connecting to device: %1").arg(modbus_strerror(errno)));
            disconnectDevice();     // no need to keep object in memory on fail
            return;
        }

        modbus_set_debug(d->libModbusClient, true);

        /* Define a new and too short timeout! */
        modbus_set_response_timeout(d->libModbusClient, responseTimeoutSec, responseTimeoutUSec),

        // update our action reader/writes
        d->readActionThread->setModbusConnection(d->libModbusClient);
        d->writeActionThread->setModbusConnection(d->libModbusClient);

        setConnectionState(CONNECTED);
    }
}

void Poller::disconnectDevice()
{
    // stop poller before disconnecting
    if (d->state == POLLING) {
        stop();
    }

    if (d->backend == QtModbusBackend) {
        if (d->modbusClient) {
            d->modbusClient->disconnectDevice();
        }

        // reset the pointer once destroyed
        connect(d->modbusClient, &QModbusClient::destroyed, [this] () {
            d->modbusClient = nullptr;
        });

        delete d->modbusClient;
    } else {
        modbus_close(d->libModbusClient);
        modbus_free(d->libModbusClient);
        d->libModbusClient = nullptr;

        // and reset the pointers to the modbus client.
        d->readActionThread->setModbusConnection(nullptr);
        d->writeActionThread->setModbusConnection(nullptr);

        setConnectionState(UNCONNECTED);
    }
}

void Poller::enqueueReadCommand(const QModbusDataUnit &readCommand)
{
    d->readQueue.data()->enqueue(readCommand);
}

void Poller::enqueueWriteCommand(const QModbusDataUnit &writeCommand)
{
    d->writeQueue.data()->enqueue(writeCommand);
}

int Poller::pollingInterval() const
{
    return d->pollTimer->interval();
}

void Poller::onLibModbusReadError(const QString &errorStr, int errNum)
{
    // notify that we've got an error so that the developer can decide what to do with it
    Q_EMIT modbusError(errNum);

    d->errorCount += 1;

    // if the error count reaches the hit mark, disconnect the serial/tcp connection and
    // notify that there's a problem
    if (d->errorCount >= 3) {
        disconnectDevice();
        Q_EMIT connectionError("Multiple read/write errors. Device disconnected: " + errorStr);

        // and reset the counter
        d->errorCount = 0;
    } else {
        // restart the timer
        d->pollTimer->start();
    }
}

void Poller::onLibModbusReplyFinished(const QModbusDataUnit &modbusReply)
{
    qDebug("[Poller::onLibModbusReplyFinished]");
    Q_EMIT dataReady(modbusReply);

    // reset the error counter
    d->errorCount = 0;
    d->pollTimer->start();
}

void Poller::onLibmodbusWriteFinished()
{
    qDebug("[Poller::onLibmodbusWriteFinished]");

    // reset the error counter
    d->errorCount = 0;
    d->pollTimer->start();
}

void Poller::onModbusReplyFinished()
{
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) {
        qDebug("NO REPLY");
        return;
    }

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        for (uint i = 0; i < unit.valueCount(); i++) {
            const QString entry = tr("Address: %1, Value: %2").arg(unit.startAddress() + i)
                                     .arg(QString::number(unit.value(i), unit.registerType() <= QModbusDataUnit::Coils ? 10 : 16));
            qDebug() << "ENTRY: " << entry;
        }

        Q_EMIT dataReady(unit);
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        qDebug() << tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16);
    } else {
        qDebug() << tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16);
    }

    reply->deleteLater();
    d->pollTimer->start();
}

void Poller::onModbusWriteReplyFinished()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) {
        qDebug("[Poller::onModbusWriteReplyFinished] NO REPLY");
        return;
    }

    if (reply->error() == QModbusDevice::ProtocolError) {
        qDebug() << tr("Write response error: %1 (Mobus exception: 0x%2)").arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16);
    } else if (reply->error() != QModbusDevice::NoError) {
        qDebug() << tr("Write response error: %1 (code: 0x%2)").arg(reply->errorString()).arg(reply->error());
    }
    reply->deleteLater();
    d->pollTimer->start();
}

void Poller::onPollTimeout()
{
    qDebug("[Poller::onPollTimeout]");

    // we give priority to the write queue
    if (!d->writeQueue.data()->isEmpty()) {
        setState(WRITING);
        writeRegister();//d->writeQueue.data()->dequeue());
    } else if (!d->readQueue.data()->isEmpty()) {
        setState(READING);
        readRegister(d->readQueue.data()->dequeue());
    } else {
        setState(POLLING);
        // run default commands
        if (d->defaultPollCommand.isValid()) {
            qDebug("Nothing to read/write... Defaulting");
            readRegister(d->defaultPollCommand);
        } else {
            qDebug("No valid default command to send");
            d->pollTimer->start();
        }
    }
}

void Poller::readRegister(int registerAddress, quint16 length)
{
    QModbusDataUnit readDU = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, registerAddress, length);
    readRegister(readDU);
}

void Poller::readRegister(const QModbusDataUnit &command)
{
    if (d->backend == QtModbusBackend) {
        if (auto *reply = d->modbusClient->sendReadRequest(command, 1)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, &Poller::onModbusReplyFinished);
            } else {
                qDebug("[Poller::readRegister] DELETING REPLY");
                delete reply; // broadcast replies return immediately
            }
        } else {
            qDebug() << "[Poller::readRegister] READ ERROR : " << d->modbusClient->errorString();
        }
    } else {
        qDebug() << "YEAH; READ COMMAND FOR LIBMODBUS!";
        // as the libmodbus implementation we made depends on 2 queues for reading and writing, we need to add the default command to the
        // read queue before starting the actual read otherwise we won't have anything to read from
        enqueueReadCommand(command);

        d->readActionThread->start();
    }
}

QModbusDataUnit Poller::prepareReadCommand(QModbusDataUnit::RegisterType type, int regAddr, quint16 readLength)
{
    return QModbusDataUnit(type, regAddr, readLength);
}

void Poller::setDefaultPollCommand(const QModbusDataUnit &defaultPollCommand)
{
    d->defaultPollCommand = defaultPollCommand;
}

//void Poller::setModbusClient(QModbusClient *modbusClient)
//{
//    if (d->pollTimer->isActive()) {
//        d->pollTimer->stop();
//    }

//    d->modbusClient = modbusClient;
//}

void Poller::setConnectionState(Poller::ConnectionState connectionState)
{
    qDebug("[Poller::setConnectionState]");

    if (d->connectionState != connectionState) {
        d->connectionState = connectionState;
        Q_EMIT connectionStateChanged(d->connectionState);
    }
}

void Poller::setState(Poller::State state)
{
    if (d->state != state) {
        d->state = state;
        Q_EMIT stateChanged(d->state);
    }
}

void Poller::setupModbusClientConnections()
{
    connect(d->modbusClient, &QModbusClient::errorOccurred, [this] (QModbusDevice::Error) {
        qDebug() << "[Poller::setupSerialConnection] MODBUS ERROR: " << d->modbusClient->errorString();
    });

    if (!d->modbusClient) {
        qDebug() << "[Poller::setupSerialConnection] Could not create Modbus client.";
    } else {
        connect(d->modbusClient, &QModbusClient::stateChanged, [this] (QModbusDevice::State state) {
            qDebug("CONNECTION STATE CHANGED! - 1 ");
            qDebug() << "TO: " << state;

            switch (state) {
                case QModbusDevice::UnconnectedState:
                    setConnectionState(UNCONNECTED);
                    break;
                case QModbusDevice::ConnectingState:
                    setConnectionState(CONNECTING);
                    break;
                case QModbusDevice::ConnectedState:
                    setConnectionState(CONNECTED);
                    break;
                case QModbusDevice::ClosingState:
                    setConnectionState(CLOSING);
                    break;
                default:
                    setConnectionState(UNCONNECTED);
                    break;
            }
        });
    }
}

void Poller::setupSerialConnection(const QString &serialPortPath, int parity, int baud, int dataBits, int stopBits, int responseTimeout, int numberOfRetries)
{
    Q_UNUSED(responseTimeout);
    Q_UNUSED(numberOfRetries);

    if (d->backend == QtModbusBackend) {
        if (d->modbusClient != nullptr) {
            qDebug("Another client is already active! Disconnecting and deleting current");

            /*
             * This action is available ONLY if the device is not connected to the board. If the device is not connected the "modbusClient" value
             * will ALWAYS be 'nullptr' UNLESS we're doing a configuration OVER a previous configuration without connecting to the board. In this
             * case (the only case in which d->modbusClient is != nullptr) we can just directly delete the object without passing through the checks in the
             * Poller::disconnectDevice function
             */

            // this won't don anything as we're never connected when it's called
            d->modbusClient->disconnectDevice();

            delete d->modbusClient;
            d->modbusClient = nullptr;
        }

        d->modbusClient = new QModbusRtuSerialMaster(this);
        d->modbusClient->setConnectionParameter(QModbusDevice::SerialPortNameParameter, serialPortPath);
        d->modbusClient->setConnectionParameter(QModbusDevice::SerialParityParameter, parity);
        d->modbusClient->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, baud);
        d->modbusClient->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, dataBits);
        d->modbusClient->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, stopBits);

        // TODO activate later
    //    d->modbusClient->setTimeout(responseTimeout);
    //    d->modbusClient->setNumberOfRetries(numberOfRetries);

        setupModbusClientConnections();
    } else {
        if (d->libModbusClient != nullptr) {
            // close/free/reset modbus pointer
            disconnectDevice();
        }

        // setup libmodbus serial settings
        d->libModbusClient = modbus_new_rtu(serialPortPath.toLatin1().constData()
                                            , baud
                                            // http://libmodbus.org/docs/v3.1.4/modbus_new_rtu.html
                                            , parity == 0 ? 'N' : parity == 1 ? 'E' : 'O'
                                            , dataBits
                                            , stopBits);
    }
}

void Poller::setupTcpConnection(const QString &hostAddress, int port, int responseTimeout, int numberOfRetries)
{
    Q_UNUSED(responseTimeout);
    Q_UNUSED(numberOfRetries);

    if (d->backend == QtModbusBackend) {
        if (d->modbusClient != nullptr) {
            qDebug("Another client is already active! Disconnecting and deleting current");

            // this won't don anything as we're never connected when it's called
            d->modbusClient->disconnectDevice();

            delete d->modbusClient;
            d->modbusClient = nullptr;
        }

        d->modbusClient = new QModbusTcpClient();
        d->modbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, hostAddress);
        d->modbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);

        // TODO activate later
    //    d->modbusClient->setTimeout(responseTimeout);
    //    d->modbusClient->setNumberOfRetries(numberOfRetries);

        // the Qt backend connections
        setupModbusClientConnections();
    } else {
        if (d->libModbusClient != nullptr) {
            disconnectDevice();
        }

        // setup libmodbus in tcp mode
        d->libModbusClient = modbus_new_tcp(hostAddress.toLatin1().constData(), port);
    }
}

void Poller::start()
{
    if (!d->pollTimer->isActive()) {
        d->pollTimer->start();

        setState(POLLING);
    }
}

Poller::State Poller::state() const
{
    return d->state;
}

void Poller::stop()
{
    if (d->pollTimer->isActive()) {
        d->pollTimer->stop();

        setState(IDLE);
    }
}

void Poller::writeRegister()
{
    qDebug("[Poller::writeRegister]");

    if (d->backend == QtModbusBackend) {
        QModbusDataUnit command = d->writeQueue.data()->dequeue();

        qDebug() << "[Poller::writeRegister] values to send: (" << command.valueCount() << " -> " << command.values();
        if (QModbusReply *reply = d->modbusClient->sendWriteRequest(command, 1)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, &Poller::onModbusWriteReplyFinished);
            } else {
                qDebug("[Poller::writeRegister] DELETING REPLY");
                delete reply; // broadcast replies return immediately
            }
        }
    } else {
        d->writeActionThread->start();
    }
}

}
