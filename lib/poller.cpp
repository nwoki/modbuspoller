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
}

void Poller::start()
{
    if (!d->pollTimer->isActive()) {
        d->pollTimer->start();
    }
}

void ModbusPoller::Poller::stop()
{
    if (d->pollTimer->isActive()) {
        d->pollTimer->stop();
    }
}

}
