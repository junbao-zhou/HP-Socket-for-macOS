#include <stdio.h>
#include <iostream>
#include <string>

#include "../../src/UdpClient.h"

class UdpClientListenerImpl: public CUdpClientListener{

public:
    //override function

    /*
    * 名称：准备连接通知
    * 描述：通信客户端组件启动时，在客户端 Socket 创建完成并开始执行连接前，Socket 监听
    *		器将收到该通知，监听器可以在通知处理方法中执行 Socket 选项设置等额外工作
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    *			socket		-- 客户端 Socket
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 终止启动通信客户端组件
    */
    virtual EnHandleResult OnPrepareConnect(IUdpClient* pSender, CONNID dwConnID, SOCKET socket) override{

        return HR_IGNORE;
    }

    /*
    * 名称：连接完成通知
    * 描述：与服务端成功建立连接时，Socket 监听器将收到该通知
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 同步连接：终止启动通信客户端组件
    *								   异步连接：关闭连接
    */
    virtual EnHandleResult OnConnect(IUdpClient* pSender, CONNID dwConnID) override{

        return HR_IGNORE;
    }


    /*
    * 名称：握手完成通知
    * 描述：连接完成握手时，Socket 监听器将收到该通知，监听器接收到该通知后才能开始
    *		数据收发操作
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 引发 OnClose() 事件并关闭连接
    */
    virtual EnHandleResult OnHandShake(IUdpClient* pSender, CONNID dwConnID) override{

        return HR_IGNORE;
    }
    /*
    * 名称：已发送数据通知
    * 描述：成功发送数据后，Socket 监听器将收到该通知
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    *			pData		-- 已发送数据缓冲区
    *			iLength		-- 已发送数据长度
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 该通知不允许返回 HR_ERROR（调试模式下引发断言错误）
    */
    virtual EnHandleResult OnSend(IUdpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength)	 override{

        return HR_IGNORE;
    }
    /*
    * 名称：数据到达通知（PUSH 模型）
    * 描述：对于 PUSH 模型的 Socket 通信组件，成功接收数据后将向 Socket 监听器发送该通知
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    *			pData		-- 已接收数据缓冲区
    *			iLength		-- 已接收数据长度
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 引发 OnClose() 事件并关闭连接
    */
    virtual EnHandleResult OnReceive(IUdpClient* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override{
        std::cout.write((const char*)pData, iLength) << std::endl;
        return HR_OK;
    }
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
    virtual EnHandleResult OnClose(IUdpClient* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)	 override{

        return HR_IGNORE;
    }
public:

    static UdpClientListenerImpl Listener;
};

UdpClientListenerImpl UdpClientListenerImpl::Listener;

int main(int argc, char *argv[])
{
    CUdpClient s(&UdpClientListenerImpl::Listener);
    if(!s.Start("127.0.0.1", 9999, false)){
        printf("create udp client error! [[%s]]\n", ::GetLastErrorStr());
        return 0;
    }

    std::string strInputTmp;
    while(true){
        std::cin >> strInputTmp;

        if(!s.Send((const BYTE*)strInputTmp.c_str(), strInputTmp.size())){
            printf("send input data error! [[%s]]\n", strInputTmp.c_str());
        }
    }
}