#include "mainwindow.h"
#include <thread>
#include <chrono>
#include "GlobalDef.h"
#include "HPTypeDef.h"
#include "SocketInterface.h"
#include "HPSocket.h"

class CListenerImpl : public CTcpServerListener
{
public:
    virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen) override
    {
        TCHAR szAddress[50];
        int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
        USHORT usPort;

        pSender->GetListenAddress(szAddress, iAddressLen, usPort);
//        LOG(INFO) << szAddress << ":" << usPort;
        return HR_OK;
    }

    virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override
    {
        BOOL bPass = TRUE;
        TCHAR szAddress[50];
        int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
        USHORT usPort;

        pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);

//        LOG(INFO) << szAddress << ":" << usPort;

        return bPass ? HR_OK : HR_ERROR;
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

};

CListenerImpl s_listener;
ITcpServer* s_server = TcpServer_Creator::Create(&s_listener);

/**
 * 该例子与client/test1.cpp测试
 */
int main(int argc, char const *argv[])
{
//    LOG(INFO) << "hello world!";
    if(!s_server->Start("", 8888)){
//        LOG(ERROR) << "开启监听失败!";
        return -1;
    }

    using namespace std;

    while(1)
        std::this_thread::sleep_for(1s);



    TcpServer_Creator::Destroy(s_server);
    return 0;
}
