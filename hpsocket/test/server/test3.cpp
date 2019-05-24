#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include "../../src/UdpServer.h"

class UdpServerListenerImpl: public CUdpServerListener{

    /*
    * 名称：通信错误通知
    * 描述：通信发生错误后，Socket 监听器将收到该通知，并关闭连接

    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    *			enOperation	-- Socket 操作类型
    *			iErrorCode	-- 错误代码
    * 返回值：	忽略返回值
    */
    virtual EnHandleResult OnClose(IUdpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override{

        return HR_IGNORE;
    }

    /*
    * 名称：准备监听通知
    * 描述：通信服务端组件启动时，在监听 Socket 创建完成并开始执行监听前，Socket 监听
    *		器将收到该通知，监听器可以在通知处理方法中执行 Socket 选项设置等额外工作
    *
    * 参数：		pSender		-- 事件源对象
    *			soListen	-- 监听 Socket
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 终止启动通信服务组件
    */
    virtual EnHandleResult OnPrepareListen(IUdpServer* pSender, SOCKET soListen) override{

        return HR_IGNORE;
    }


    /*
    * 名称：接收连接通知
    * 描述：接收到客户端连接请求时，Socket 监听器将收到该通知，监听器可以在通知处理方
    *		法中执行 Socket 选项设置或拒绝客户端连接等额外工作
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    *			soClient	-- TCP: 客户端 Socket 句柄，UDP: 客户端 Socket SOCKADDR 指针
    * 返回值：	HR_OK / HR_IGNORE	-- 接受连接
    *			HR_ERROR			-- 拒绝连接
    */
    virtual EnHandleResult OnAccept(IUdpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override{

        return HR_IGNORE;
    }


    /*
    * 名称：已发送数据通知
    * 描述：成功发送数据后，Socket 监听器将收到该通知
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 该通知不允许返回 HR_ERROR（调试模式下引发断言错误）
    */
    virtual EnHandleResult OnSend(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override{

        return HR_IGNORE;
    }

    /*
    * 名称：数据到达通知（PUSH 模型）
    * 描述：对于 PUSH 模型的 Socket 通信组件，成功接收数据后将向 Socket 监听器发送该通知
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 引发 OnClose() 事件并关闭连接
    */
    virtual EnHandleResult OnReceive(IUdpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override{

        if(!pSender->Send(dwConnID, pData, iLength))
            return HR_ERROR;
        return HR_IGNORE;
    }

public:
    static UdpServerListenerImpl Listener;
};

UdpServerListenerImpl UdpServerListenerImpl::Listener;

/**
 * 该例子与client/test5.cpp测试
 */
int main(int argc, char *argv[])
{
    CUdpServer s(&UdpServerListenerImpl::Listener);
    if(!s.Start(nullptr, 9999)){
        printf("create udp server listen error!\n");
        return 0;
    }

	
	while(1)
		std::this_thread::sleep_for(1s);

    return 0;
}