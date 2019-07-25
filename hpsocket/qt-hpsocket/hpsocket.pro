QT -= core gui

CONFIG += c++14 staticlib
TEMPLATE = lib

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#暂时关闭对ssl的支持
DEFINES += _SSL_DISABLED
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000   QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12 # disables all the APIs deprecated before Qt 6.0.0


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../src/common/http/Readme.txt \
    ../src/common/kcp/Readme.txt

HEADERS += \
    ../src/common/crypto/Crypto.h \
    ../src/common/http/http_parser.h \
    ../src/common/kcp/ikcp.h \
    ../src/common/BufferPool.h \
    ../src/common/BufferPtr.h \
    ../src/common/CriSec.h \
    ../src/common/Event.h \
    ../src/common/FileHelper.h \
    ../src/common/FuncHelper.h \
    ../src/common/GeneralHelper.h \
    ../src/common/GlobalDef.h \
    ../src/common/GlobalErrno.h \
    ../src/common/IODispatcher.h \
    ../src/common/MessagePipe.h \
    ../src/common/PollHelper.h \
    ../src/common/PrivateHeap.h \
    ../src/common/RingBuffer.h \
    ../src/common/RWLock.h \
    ../src/common/Semaphore.h \
    ../src/common/SignalHandler.h \
    ../src/common/Singleton.h \
    ../src/common/STLHelper.h \
    ../src/common/StringT.h \
    ../src/common/SysHelper.h \
    ../src/common/Thread.h \
    ../src/common/TimerPipe.h \
    ../src/ArqHelper.h \
    ../src/HPSocket-SSL.h \
    ../src/HPSocket.h \
    ../src/HPThreadPool.h \
    ../src/HPTypeDef.h \
    ../src/HttpAgent.h \
    ../src/HttpClient.h \
    ../src/HttpCookie.h \
    ../src/HttpHelper.h \
    ../src/HttpServer.h \
    ../src/MiscHelper.h \
    ../src/SocketHelper.h \
    ../src/SocketInterface.h \
    ../src/SocketObject4C.h \
    ../src/SSLAgent.h \
    ../src/SSLClient.h \
    ../src/SSLHelper.h \
    ../src/SSLServer.h \
    ../src/TcpAgent.h \
    ../src/TcpClient.h \
    ../src/TcpPackAgent.h \
    ../src/TcpPackClient.h \
    ../src/TcpPackServer.h \
    ../src/TcpPullAgent.h \
    ../src/TcpPullClient.h \
    ../src/TcpPullServer.h \
    ../src/TcpServer.h \
    ../src/UdpArqClient.h \
    ../src/UdpArqServer.h \
    ../src/UdpCast.h \
    ../src/UdpClient.h \
    ../src/UdpServer.h \
    ../test/helper.h

SOURCES += \
    ../src/common/crypto/Crypto.cpp \
    ../src/common/BufferPool.cpp \
    ../src/common/Event.cpp \
    ../src/common/FileHelper.cpp \
    ../src/common/FuncHelper.cpp \
    ../src/common/IODispatcher.cpp \
    ../src/common/MessagePipe.cpp \
    ../src/common/PollHelper.cpp \
    ../src/common/RWLock.cpp \
    ../src/common/SysHelper.cpp \
    ../src/common/Thread.cpp \
    ../src/common/TimerPipe.cpp \
    ../src/ArqHelper.cpp \
    ../src/HPSocket-SSL.cpp \
    ../src/HPSocket.cpp \
    ../src/HPThreadPool.cpp \
    ../src/HttpAgent.cpp \
    ../src/HttpClient.cpp \
    ../src/HttpCookie.cpp \
    ../src/HttpHelper.cpp \
    ../src/HttpServer.cpp \
    ../src/MiscHelper.cpp \
    ../src/SocketHelper.cpp \
    ../src/SSLAgent.cpp \
    ../src/SSLClient.cpp \
    ../src/SSLHelper.cpp \
    ../src/SSLServer.cpp \
    ../src/TcpAgent.cpp \
    ../src/TcpClient.cpp \
    ../src/TcpPackAgent.cpp \
    ../src/TcpPackClient.cpp \
    ../src/TcpPackServer.cpp \
    ../src/TcpPullAgent.cpp \
    ../src/TcpPullClient.cpp \
    ../src/TcpPullServer.cpp \
    ../src/TcpServer.cpp \
    ../src/UdpArqClient.cpp \
    ../src/UdpArqServer.cpp \
    ../src/UdpCast.cpp \
    ../src/UdpClient.cpp \
    ../src/UdpServer.cpp \
    ../src/common/http/http_parser.c \
    ../src/common/kcp/ikcp.c \
    ../test/helper.cpp \
    ../test/server/test2.cpp

#ssl的依赖库
#macx: LIBS += -L/opt/local/lib -lssl -lcrypto -lglog

INCLUDEPATH += /opt/local/include
