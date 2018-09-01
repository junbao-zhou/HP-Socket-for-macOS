#include <QCoreApplication>

#include "TcpPackServer.h"

class ServerImpl: public CTcpServerListener{
public:
    virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen) override
    {
        return HR_OK;
    }

    virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override
    {
        return HR_OK;
    }

    virtual EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID) override
    {
        return HR_OK;
    }

    virtual EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        if(pSender->Send(dwConnID, pData, iLength))
            return HR_OK;

        return HR_ERROR;
    }

    virtual EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
    {
        return HR_OK;
    }

    virtual EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override
    {
        return HR_OK;
    }

    virtual EnHandleResult OnShutdown(ITcpServer* pSender) override
    {
        return HR_OK;
    }

public:
    static ServerImpl instance;
};

ServerImpl ServerImpl::instance;

CTcpPackServer g_tcpPackServer(&ServerImpl::instance);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    g_tcpPackServer.Start(nullptr, 9999);

    return a.exec();
}
