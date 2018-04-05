#include "poller.h"
#include "poller_p.h"

#include <QtCore/QDebug>

#include <QtSerialBus/QModbusClient>


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
    Q_EMIT stateChanged(d->state);
}

Poller::~Poller()
{
    delete d;
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

void Poller::onPollTimeout()
{
    qDebug("[Poller::onPollTimeout]");

    // we give priority to the write queue
    if (!d->writeQueue.isEmpty()) {
        // TODO
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

void Poller::setModbusClient(QModbusClient *modbusClient)
{
    if (d->pollTimer->isActive()) {
        d->pollTimer->stop();
    }

    d->modbusClient = modbusClient;
}

void Poller::setState(Poller::State state)
{
    if (d->state != state) {
        d->state = state;
        Q_EMIT stateChanged(d->state);
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
    }
}

}
