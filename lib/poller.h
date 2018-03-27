#ifndef MODBUSPOLLER_POLLER_H
#define MODBUSPOLLER_POLLER_H

#include "poller_global.h"

#include <QtCore/QObject>

#include <QtSerialBus/QModbusDataUnit>

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
    explicit Poller(const QModbusDataUnit &defaultCommand = QModbusDataUnit(), quint16 pollingInterval = 1000, QObject *parent = nullptr);
    virtual ~Poller();

    /**
     * @brief enqueueReadCommand
     * @param readCommand the QModBusDataUnit command to send to the queue
     */
    void enqueueReadCommand(const QModbusDataUnit &readCommand);

    /**
     * @brief enqueueWriteCommand
     * @param writeCommand the QModBusDataUnit command to send to the queue
     */
    void enqueueWriteCommand(const QModbusDataUnit &writeCommand);

    /**
     * @brief prepareReadCommand
     * @param the type of reading/writing to do
     * @param regAddr the register address to read from
     * @param readLength the length to read for
     */
    static QModbusDataUnit prepareReadCommand(QModbusDataUnit::RegisterType type, int regAddr, quint16 readLength);

    void setDefaultPollCommand(const QModbusDataUnit &defaultPollCommand);
    void setModbusClient(QModbusClient *modbusClient);

    /** start polling */
    void start();

    /** stop polling */
    void stop();

protected:
    /**
     * @brief dataReady - called when a read request is successful
     * @param readData data object with the response from a read command
     */
    virtual void dataReady(const QModbusDataUnit &readData) = 0;

private Q_SLOTS:
    void onModbusReplyFinished();
    void onPollTimeout();

private:
    /**
     * @brief readRegister
     * @param registerAddress the address to read from
     * @param length the number of registers to read subsequentially from the @ref registerAddress
     */
    void readRegister(int registerAddress, quint16 length);
    void readRegister(const QModbusDataUnit &command);

    PollerPrivate * const d;
};


}

#endif // MODBUSPOLLER_POLLER_H
