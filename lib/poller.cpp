#include "poller.h"
#include "poller_p.h"

#include <QtCore/QDebug>

#include <QtSerialBus/QModbusClient>
#include <QtSerialBus/QModbusRtuSerialMaster>


namespace ModbusPoller {

Poller::Poller(const QModbusDataUnit &defaultCommand, quint16 pollingInterval, QObject *parent)
    : QObject(parent)
    , d(new PollerPrivate)
{
    d->defaultPollCommand = defaultCommand;
    d->pollTimer->setInterval(pollingInterval);
    d->pollTimer->setSingleShot(true);

    connect(d->pollTimer, &QTimer::timeout, this, &Poller::onPollTimeout);

    // notify starting state
    Q_EMIT connectionStateChanged(d->connectionState);
    Q_EMIT stateChanged(d->state);
}

Poller::~Poller()
{
    delete d;
}

void Poller::connectDevice()
{
    if (!d->modbusClient) {
        qDebug("[Poller::connectDevice] modbus client not configured!");
        return;
    }

    if (!d->modbusClient->connectDevice()) {
        qDebug() << "[Poller::connectDevice] CONNECTION ERROR: " << d->modbusClient->errorString();
    }

    qDebug("ALl is well. Start polling");
}

void Poller::disconnectDevice()
{
    // stop poller before disconnecting
    if (d->state == POLLING) {
        stop();
    }

    if (d->modbusClient) {
        d->modbusClient->disconnectDevice();
    }

    // reset the pointer once destroyed
    connect(d->modbusClient, &QModbusClient::destroyed, [this] () {
        d->modbusClient = nullptr;
    });

    delete d->modbusClient;
}

void Poller::enqueueReadCommand(const QModbusDataUnit &readCommand)
{
    d->readQueue.enqueue(readCommand);
}

void Poller::enqueueWriteCommand(const QModbusDataUnit &writeCommand)
{
    d->writeQueue.enqueue(writeCommand);
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
    if (!d->writeQueue.isEmpty()) {
        setState(WRITING);
        writeRegister(d->writeQueue.dequeue());
    } else if (!d->readQueue.isEmpty()) {
        setState(POLLING);
        readRegister(d->readQueue.dequeue());
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

void Poller::setupSerialConnection(const QString &serialPortPath, int parity, int baud, int dataBits, int stopBits, int responseTimeout, int numberOfRetries)
{
    Q_UNUSED(responseTimeout);
    Q_UNUSED(numberOfRetries);

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

void Poller::start()
{
    if (!d->pollTimer->isActive()) {
        d->pollTimer->start();

        setState(POLLING);
    }
}

void Poller::stop()
{
    if (d->pollTimer->isActive()) {
        d->pollTimer->stop();

        setState(IDLE);
    }
}

void Poller::writeRegister(const QModbusDataUnit &command)
{
    qDebug("[Poller::writeRegister]");
    qDebug() << "[Poller::writeRegister] values to send: (" << command.valueCount() << " -> " << command.values();


    if (QModbusReply *reply = d->modbusClient->sendWriteRequest(command, 1)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, &Poller::onModbusWriteReplyFinished);
        } else {
            qDebug("[Poller::writeRegister] DELETING REPLY");
            delete reply; // broadcast replies return immediately
        }
    }
}

}
