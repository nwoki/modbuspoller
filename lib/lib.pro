QT += core serialbus

TARGET = modbuspoller
TEMPLATE = lib

DEFINES += MODBUSPOLLER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    poller.cpp \
    ../3rdparty/libmodbus-3.1.4/src/modbus.c \
    ../3rdparty/libmodbus-3.1.4/src/modbus-data.c \
    ../3rdparty/libmodbus-3.1.4/src/modbus-rtu.c

HEADERS += \
    poller.h \
    poller_global.h \
    poller_p.h \
    ../3rdparty/libmodbus-3.1.4/src/modbus.h

INCLUDEPATH += \
    ../3rdparty/libmodbus-3.1.4 \
    ../3rdparty/libmodbus-3.1.4/src

unix {
    isEmpty(PREFIX): PREFIX = /usr
    target.path = $${PREFIX}/lib
    INSTALLS += target
}
