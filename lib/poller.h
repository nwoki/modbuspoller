#ifndef MODBUSPOLLER_POLLER_H
#define MODBUSPOLLER_POLLER_H

#include "poller_global.h"

#include <QtCore/QObject>

class QModbusClient;


namespace ModbusPoller {

class PollerPrivate;


/**
 * @brief The Poller class
 * @author Francesco Nwokeka <francesco.nwokeka@rawfish.it>
 *
 * Poller class used to poll info from the board
 */
class Poller : public QObject
{
    Q_OBJECT

public:
    explicit Poller(quint16 pollingInterval = 1000, QObject *parent = nullptr);
    ~Poller();

    /**
     * @brief readRegister
     * @param registerAddress the address to read from
     * @param length the number of registers to read subsequentially from the @ref registerAddress
     */
    void readRegister(int registerAddress, quint16 length);

    void setModbusClient(QModbusClient *modbusClient);

    /** start polling */
    void start();

    /** stop polling */
    void stop();

private Q_SLOTS:
    void onModbusReplyFinished();
    void onPollTimeout();

private:
    PollerPrivate * const d;
};


}

#endif // MODBUSPOLLER_POLLER_H
