#include <glog/logging.h>
#include "../../src/TcpPackServer.h"
#include <thread>

class CListenerImpl : public CTcpServerListener
{
public:
	virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen) override
	{
		TCHAR szAddress[50];
		int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
		USHORT usPort;

		pSender->GetListenAddress(szAddress, iAddressLen, usPort);
        LOG(INFO) << szAddress << ":" << usPort;
		return HR_OK;
	}

	virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override
	{
		BOOL bPass = TRUE;
		TCHAR szAddress[50];
		int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
		USHORT usPort;

		pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);

        LOG(INFO) << szAddress << ":" << usPort;

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
CTcpPackServer s_server(&s_listener);

/**
 * 该例子与client/test2.cpp测试
 */
int main(int argc, char const *argv[])
{
    LOG(INFO) << "hello world!";
    s_server.SetMaxPackSize(0x7FF);
	s_server.SetPackHeaderFlag(0x1F9);
    
	if(!s_server.Start("", 8888)){
        LOG(ERROR) << "开启监听失败!";
		return -1;
	}
	

	while(1)
		std::this_thread::sleep_for(1s);


    return 0;
}
