QT -= gui
QT += core

CONFIG += c++14 console
CONFIG -= app_bundle
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
    common/BufferPool.cpp \
    common/FuncHelper.cpp \
    common/SysHelper.cpp \
    common/Thread.cpp \
    common/BufferPool.cpp \
    common/TItem.cpp \
    common/TSimpleList.cpp \
    common/CNodePoolT.cpp \
    common/TItemList.cpp \
    common/TItemListExT.cpp \
    common/TItemPtr.cpp \
    common/TBuffer.cpp \
    common/CBufferPool.cpp \
    common/Event.cpp \
    common/MessagePipe.cpp \
    common/TimerPipe.cpp \
    common/PollHelper.cpp \
    common/FileHelper.cpp \
    common/RWLock.cpp \
    common/IODispatcher.cpp \
    SocketHelper.cpp \
    TcpServer.cpp \
    helper.cpp \
    TcpClient.cpp \
    UdpServer.cpp \
    UdpClient.cpp

DISTFILES += \
    TestCode.txt

HEADERS += \
    common/BufferPool.h \
    common/FuncHelper.h \
    common/SysHelper.h \
    common/Thread.h \
    common/RingBuffer.h \
    common/BufferPool.h \
    common/TItem.h \
    common/TSimpleList.h \
    common/CNodePoolT.h \
    common/TItemList.h \
    common/TItemListExT.h \
    common/TItemPtr.h \
    common/TBuffer.h \
    common/CBufferPool.h \
    common/BufferPtr.h \
    common/CriSec.h \
    common/GlobalDef.h \
    common/Event.h \
    common/MessagePipe.h \
    common/TimerPipe.h \
    common/FileHelper.h \
    common/GeneralHelper.h \
    common/GlobalErrno.h \
    common/PrivateHeap.h \
    common/RWLock.h \
    common/Semaphore.h \
    common/STLHelper.h \
    common/StringT.h \
    common/SignalHandler.h \
    common/Singleton.h \
    common/IODispatcher.h \
    SocketHelper.h \
    SocketInterface.h \
    TcpServer.h \
    helper.h \
    TcpClient.h \
    UdpServer.h \
    UdpClient.h
