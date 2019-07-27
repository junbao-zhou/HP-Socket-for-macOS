#ifndef TCPSERVERIMPL_H
#define TCPSERVERIMPL_H

#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <QString>
//将hpsocket的头文件放最下面
#include "HPTypeDef.h"
#include "SocketInterface.h"
#include "HPSocket.h"

class TcpServerListenerImpl : public CTcpServerListener
{
    // CTcpServerListener interface

    // ISocketListenerT interface
public:
    virtual EnHandleResult OnSend(ITcpServer *pSender, CONNID dwConnID, const BYTE *pData, int iLength) override;
    virtual EnHandleResult OnReceive(ITcpServer *pSender, CONNID dwConnID, const BYTE *pData, int iLength) override;
    virtual EnHandleResult OnClose(ITcpServer *pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override;

    std::list<QString> getExterMsg();

private:
    std::list<QString> m_listRecvMsg;
    std::list<QString> m_listExterMsg;
    std::mutex m_mtx;
};

#endif // TCPSERVERIMPL_H
