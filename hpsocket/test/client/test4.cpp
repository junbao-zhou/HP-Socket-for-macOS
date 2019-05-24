#include <iostream>
#include "../../src/TcpPackAgent.h"
#include <memory>

class ServerImpl: public CTcpAgentListener{
public:

    virtual EnHandleResult OnPrepareConnect(ITcpAgent* pSender, CONNID dwConnID, SOCKET socket){
        return HR_IGNORE;
    }
    virtual EnHandleResult OnConnect(ITcpAgent* pSender, CONNID dwConnID){
        return HR_IGNORE;
    }
    virtual EnHandleResult OnHandShake(ITcpAgent* pSender, CONNID dwConnID){
        return HR_IGNORE;
    }
    virtual EnHandleResult OnSend(ITcpAgent* pSender, CONNID dwConnID, const BYTE* pData, int iLength){
        return HR_IGNORE;
    }
    virtual EnHandleResult OnShutdown(ITcpAgent* pSender){
        return HR_IGNORE;
    }

    virtual EnHandleResult OnReceive(ITcpAgent* pSender, CONNID dwConnID, const BYTE* pData, int iLength){
        if(iLength > 0)
            std::cout.write((const char *)pData, iLength) << std::endl;
        return HR_IGNORE;
    }

    virtual EnHandleResult OnClose(ITcpAgent* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode){
        return HR_IGNORE;
    }
public:
    static ServerImpl instance;
};

ServerImpl ServerImpl::instance;

CTcpPackAgent g_tcpPackAgentClient(&ServerImpl::instance);

/**
 * 该例子与server/test1.cpp测试
 */
int main(int argc, char *argv[])
{

    g_tcpPackAgentClient.SetMaxPackSize(0x7FF);
	g_tcpPackAgentClient.SetPackHeaderFlag(0x1F9);
    if(false == g_tcpPackAgentClient.Start(nullptr, false)){
        printf("start agent error!");
        std::exit(0);
    }

    CONNID dwIpAddress;
    if(false == g_tcpPackAgentClient.Connect("127.0.0.1", 8888, &dwIpAddress)){
        printf("agent connect to server 118.25.7.178:8888 error!");
        std::exit(0);
    }
    printf("IpAddress: %d\n", dwIpAddress);

    do{
        std::string strTmp;
        std::cin >> strTmp;

        if(strTmp.empty())
            continue;

        if(strTmp.compare("#quit") == 0)
            break;

        if(g_tcpPackAgentClient.Send(dwIpAddress, (BYTE *)(strTmp.data()), strTmp.size()))
            continue;

        printf("write data error!\n");
    }while(true);


    printf("bye bye~\n");
    return 0;
}
