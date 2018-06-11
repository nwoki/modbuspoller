# MODBUS POLLER

Wrapper around the QtModbus modules and libmodbus for modbus communication. The library was made in order to facilitate modbus communication and setup for Qt/C++ projects without having to go through all the fuss.
Current


## Requirements

* Qt 5.10.x
* gcc compiler / mingw (windows)


## OS

* Windows
* Linux

## Compile

```
qmake
make
```


## How to use

The main class you'll be using it the `ModbusPoller::Poller` class. This class has the task of taking care of your requests and handling the modbus connection. All you'll have to worry about is creating the requests and reading the responses.

When creating the `Poller` class (from which you'll have to inherit to handle the `ModbusPoller::Poller::dataReady` function for when read data is read from the modbus protocol) you can specify the following:

```
explicit Poller(Backend backend, const QModbusDataUnit &defaultCommand = QModbusDataUnit(), quint16 pollingInterval = 1000, QObject *parent = nullptr);
```

* **backend:** enum value that let's you decide which backend to use. Be it `Qt` or `libmodbus`
* **defaultCommand:** the default command to issue on every poll timeout. This is useful for monitoring purposes
* **pollingInterval:** the interval[msec] with which to poll the board

Once the poller class is setup, in order to connect to a device you'll have to configur it. Be it via serial or tcp (`Poller::setupSerialConnection` & `Poller::setupTcpConnection`).

Once this is done you can now connect to your device with `Poller::connectDevice` and close the connection with `Poller::disconnectDevice`.

Once your connection is established, the read/write requests are handle with 2 queues: read/write queue. In order to get a read/write action through, you'll have to enqueue a `QModbusDataUnit` with the register(s) to read or write via the predefine methods:

* `Poller::enqueueReadCommand`
* `Poller::enqueueWriteCommand`

and listen to the `Poller::dataReady` signal to handle the responses.

You won't need to handle the connection as it's all taken care of in the library. Just configure, connect and enqueue read/write commands and you're done.

Happy coding!





> **NOTE:** this library is still WIP. There are still a lot of things to get done before it being fully production ready:
>
* error handling
* tcp testing
* unit tests
>
>
> If you have ideas or find any bugs, please feel free to report them or send in a patch
