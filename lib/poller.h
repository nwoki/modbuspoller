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
 * @author Francesco Nwokeka <francesco.nwokeka@gmail.com>
 *
 * Poller class used to poll info from the board
 */
class MODBUSPOLLERSHARED_EXPORT Poller : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief The Backend enum
     * Tells the poller which backend to use for comunicating with the modbus device
     */
    enum Backend {
        QtModbusBackend,
        LibModbusBackend
    };
    Q_ENUM(Backend)


    /**
     * @brief The ConnectionState enum
     * This enum reflects the Qts connection state enum from QModbusDevice::State
     */
    enum ConnectionState {
        /** the modbus device is not connected */
        UNCONNECTED = 0,

        /** the modbus device is trying to connect */
        CONNECTING,

        /** the modbus device is connected */
        CONNECTED,

        /** the modbus device is closing its connection */
        CLOSING
    };
    Q_ENUM(ConnectionState)


    /**
     * @brief The State enum
     * enumerates the different states the poller can assume
     */
    enum State {
        /** poller is doing nothing */
        IDLE = 0,

        /** poller is reading from the board */
        POLLING,

        /** poller is writing info to the board */
        WRITING
    };
    Q_ENUM(State)


    explicit Poller(Backend backend, const QModbusDataUnit &defaultCommand = QModbusDataUnit(), quint16 pollingInterval = 1000, QObject *parent = nullptr);
    virtual ~Poller();

    /** establishes a modbus connection */
    void connectDevice();

    /** disconnects the current modbus connection */
    void disconnectDevice();

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
    // DEPRECATED
//    void setModbusClient(QModbusClient *modbusClient);

    void setupSerialConnection(const QString &serialPortPath, int parity, int baud, int dataBits, int stopBits, int responseTimeout = 200, int numberOfRetries = 3);
    void setupTcpConnection(const QString &hostAddress, int port, int responseTimeout = 200, int numberOfRetries = 3);

    /** start polling */
    void start();

    /** stop polling */
    void stop();

Q_SIGNALS:
    void connectionError(const QString &errorStr);
    void connectionStateChanged(Poller::ConnectionState connectionState);
    void stateChanged(Poller::State state); // todo - rename to "pollerState"

protected:
    /**
     * @brief dataReady - called when a read request is successful
     * @param readData data object with the response from a read command
     */
    virtual void dataReady(const QModbusDataUnit &readData) = 0;

private Q_SLOTS:
    void onLibModbusReplyFinished(const QModbusDataUnit &modbusReply);
    void onLibmodbusWriteFinished();
    void onModbusReplyFinished();
    void onModbusWriteReplyFinished();
    void onPollTimeout();

private:
    /**
     * @brief readRegister
     * @param registerAddress the address to read from
     * @param length the number of registers to read subsequentially from the @ref registerAddress
     */
    void readRegister(int registerAddress, quint16 length);
    void readRegister(const QModbusDataUnit &command);

    void setupModbusClientConnections();
    void setConnectionState(Poller::ConnectionState connectionState);
    void setState(Poller::State state);

    void writeRegister();

    PollerPrivate * const d;
};


}

#endif // MODBUSPOLLER_POLLER_H
