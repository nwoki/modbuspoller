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
    actionthread.cpp \
    readactionthread.cpp \
    writeactionthread.cpp

HEADERS += \
    poller.h \
    poller_global.h \
    poller_p.h \
    ../3rdparty/libmodbus-3.1.4/src/modbus.h \
    actionthread.h \
    readactionthread.h \
    writeactionthread.h

INCLUDEPATH += \
    ../3rdparty/libmodbus-3.1.4 \
    ../3rdparty/libmodbus-3.1.4/src

unix {
SOURCES += \
    ../3rdparty/libmodbus-3.1.4/src/modbus.c \
    ../3rdparty/libmodbus-3.1.4/src/modbus-data.c \
    ../3rdparty/libmodbus-3.1.4/src/modbus-rtu.c \
    ../3rdparty/libmodbus-3.1.4/src/modbus-tcp.c \

    isEmpty(PREFIX): PREFIX = /usr
    target.path = $${PREFIX}/lib
    INSTALLS += target
}

# pre-compiled version of libmodbus 3.1.4. Was compiled with mingw
#win32: LIBS += -L$$PWD/../3rdparty/win/ -llibmodbus.dll

win32 {
    LIBS += -L$$PWD/../3rdparty/win/ -llibmodbus.dll
    INCLUDEPATH += $$PWD/../3rdparty/win
    DEPENDPATH += $$PWD/../3rdparty/win
}
