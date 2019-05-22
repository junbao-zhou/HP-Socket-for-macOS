#include <glog/logging.h>
#include "../../src/TcpClient.h"
#include <thread>
#include <string>

class CListenerImpl : public CTcpClientListener
{

public:
	virtual EnHandleResult OnPrepareConnect(ITcpClient* pSender, CONNID dwConnID, SOCKET socket) override
	{
		return HR_OK;
	}

	virtual EnHandleResult OnConnect(ITcpClient* pSender, CONNID dwConnID) override
	{
		TCHAR szAddress[50];
		int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
		USHORT usPort;

		pSender->GetRemoteHost(szAddress, iAddressLen, usPort);

		return HR_OK;
	}

	virtual EnHandleResult OnHandShake(ITcpClient* pSender, CONNID dwConnID) override
	{
		return HR_OK;
	}

	virtual EnHandleResult OnReceive(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
	{
		return HR_OK;
	}

	virtual EnHandleResult OnSend(ITcpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override
	{
		return HR_OK;
	}

	virtual EnHandleResult OnClose(ITcpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override
	{
		return HR_OK;
	}

};

CListenerImpl s_listener;
CTcpClient s_client(&s_listener);

#include <iostream>

int main(int argc, char const *argv[])
{
    LOG(INFO) << "hello world!";
	if(!s_client.Start("127.0.0.1", 8888, false)){
		LOG(ERROR) << "开启监听失败!";
		return -1;
	}
	
    std::string strInput;
	while(1){
        std::cout << "输入一行数据: " << std::endl;
        std::getline(std::cin, strInput);
        if(!strInput.empty()){
            s_client.Send((const BYTE*)strInput.c_str(), strInput.size());
        }
    }

    return 0;
}
