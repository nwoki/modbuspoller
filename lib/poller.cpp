#include "poller.h"
#include "poller_p.h"

#include <QtCore/QDebug>

#include <QtSerialBus/QModbusClient>
#include <QtSerialBus/QModbusDataUnit>


namespace ModbusPoller {

Poller::Poller(quint16 pollingInterval, QObject *parent)
    : QObject(parent)
    , d(new PollerPrivate)
{
    d->pollTimer->setInterval(pollingInterval);
    d->pollTimer->setSingleShot(true);

    connect(d->pollTimer, &QTimer::timeout, this, &Poller::onPollTimeout);
}

Poller::~Poller()
{
    delete d;
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
            qDebug() << "Value: " << unit.value(i);
        }
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


    // tmp. For now just read. Next step, use enqueuing
    readRegister(512, 10);
}

void Poller::readRegister(int registerAddress, quint16 length)
{
    QModbusDataUnit readDU = QModbusDataUnit(QModbusDataUnit::HoldingRegisters, registerAddress, length);

    if (auto *reply = d->modbusClient->sendReadRequest(readDU, 1)) {
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

void Poller::setModbusClient(QModbusClient *modbusClient)
{
    if (d->pollTimer->isActive()) {
        d->pollTimer->stop();
    }

    d->modbusClient = modbusClient;
}

void Poller::start()
{
    if (!d->pollTimer->isActive()) {
        d->pollTimer->start();
    }
}

void Poller::stop()
{
    if (d->pollTimer->isActive()) {
        d->pollTimer->stop();
    }
}

}
