#include <QCoreApplication>
#include <iostream>
#include "TcpAgent.h"
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
//    virtual EnHandleResult OnReceive(ITcpAgent* pSender, CONNID dwConnID, int iLength){
//        return HR_IGNORE;
//    }
    virtual EnHandleResult OnSend(ITcpAgent* pSender, CONNID dwConnID, const BYTE* pData, int iLength){
        return HR_IGNORE;
    }
    virtual EnHandleResult OnShutdown(ITcpAgent* pSender){
        return HR_IGNORE;
    }

//    virtual EnHandleResult OnPrepareConnect(ITcpAgent* pSender, CONNID dwConnID, SOCKET socket){
//        return HR_IGNORE;
//    }

//    virtual EnHandleResult OnConnect(ITcpAgent* pSender, CONNID dwConnID){
//        return HR_IGNORE;
//    }


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
//    virtual EnHandleResult OnHandShake(ITcpAgent* pSender, CONNID dwConnID){
//        return HR_IGNORE;
//    }
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
//    virtual EnHandleResult OnSend(ITcpAgent* pSender, CONNID dwConnID, const BYTE* pData, int iLength){
//        return HR_IGNORE;
//    }
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
    virtual EnHandleResult OnReceive(ITcpAgent* pSender, CONNID dwConnID, const BYTE* pData, int iLength){
        if(iLength > 0){
            std::cout.write((const char *)pData, iLength);
            std::cout << std::endl;
        }
        return HR_IGNORE;
    }
    /*
    * 名称：数据到达通知（PULL 模型）
    * 描述：对于 PULL 模型的 Socket 通信组件，成功接收数据后将向 Socket 监听器发送该通知
    *
    * 参数：		pSender		-- 事件源对象
    *			dwConnID	-- 连接 ID
    *			iLength		-- 已接收数据长度
    * 返回值：	HR_OK / HR_IGNORE	-- 继续执行
    *			HR_ERROR			-- 引发 OnClose() 事件并关闭连接
    */
//    virtual EnHandleResult OnReceive(ITcpAgent* pSender, CONNID dwConnID, int iLength){
//        return HR_IGNORE;
//    }
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
    virtual EnHandleResult OnClose(ITcpAgent* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode){
        return HR_IGNORE;
    }
public:
    static ServerImpl instance;
};

ServerImpl ServerImpl::instance;

CTcpAgent g_tcpAgentClient(&ServerImpl::instance);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//    g_tcpPackServer.Start(nullptr, 9999);
    if(false == g_tcpAgentClient.Start(nullptr, false)){
        printf("start agent error!");
        std::exit(0);
    }

    CONNID dwIpAddress;
    if(false == g_tcpAgentClient.Connect("118.25.7.178", 5555, &dwIpAddress)){
        printf("agent connect to server 118.25.7.178:5555 error!");
        std::exit(0);
    }

    do{
        std::string strTmp;
        std::cin >> strTmp;

        if(strTmp.empty())
            continue;

        if(strTmp.compare("#quit") == 0)
            break;

        if(g_tcpAgentClient.Send(dwIpAddress, (BYTE *)(strTmp.c_str()), strTmp.size()))
            continue;

    }while(true);


    printf("bye bye~\n");
    return a.exec();
}
