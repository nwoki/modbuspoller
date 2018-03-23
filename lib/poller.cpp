#include "poller.h"
#include "poller_p.h"

namespace ModbusPoller {

Poller::Poller(quint16 pollingInterval, QObject *parent)
    : QObject(parent)
    , d(new PollerPrivate)
{
    d->pollTimer->setInterval(pollingInterval);

    connect(d->pollTimer, &QTimer::timeout, this, &Poller::onPollTimeout);
}

Poller::~Poller()
{
    delete d;
}

void Poller::onPollTimeout()
{
    qDebug("[Poller::onPollTimeout]");
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
