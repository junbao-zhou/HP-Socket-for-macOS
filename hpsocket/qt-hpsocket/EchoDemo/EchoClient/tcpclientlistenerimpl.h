#ifndef TCPCLIENTLISTENERIMPL_H
#define TCPCLIENTLISTENERIMPL_H
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <QString>
//将hpsocket的头文件放最下面
#include "HPTypeDef.h"
#include "SocketInterface.h"
#include "HPSocket.h"

class TcpClientListenerImpl : public CTcpClientListener
{
    // ISocketListenerT interface
public:
    virtual EnHandleResult OnReceive(ITcpClient *pSender, CONNID dwConnID, const BYTE *pData, int iLength) override;
    virtual EnHandleResult OnClose(ITcpClient *pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override;

    std::list<QString> getExterMsg();

private:
    std::list<QString> m_listRecvMsg;
    std::list<QString> m_listExterMsg;
    std::mutex m_mtx;
};

#endif // TCPCLIENTLISTENERIMPL_H
