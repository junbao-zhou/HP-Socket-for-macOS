#include "tcpclientlistenerimpl.h"

EnHandleResult TcpClientListenerImpl::OnReceive(ITcpClient *pSender, CONNID dwConnID, const BYTE *pData, int iLength)
{
    //接收到的响应

    if(m_mtx.try_lock()){
        if(!m_listRecvMsg.empty()){
            m_listExterMsg.splice(m_listExterMsg.end(), std::move(m_listRecvMsg));
        }
        m_listExterMsg.emplace_back(QString::fromLocal8Bit((const char*)pData, iLength));
        m_mtx.unlock();
    }else{
        m_listRecvMsg.emplace_back(QString::fromLocal8Bit((const char*)pData, iLength));
    }

    return HR_OK;
}

EnHandleResult TcpClientListenerImpl::OnClose(ITcpClient *pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
    return HR_OK;
}

std::list<QString> TcpClientListenerImpl::getExterMsg()
{
    std::list<QString> msg;
    if(m_mtx.try_lock()){
        msg.splice(msg.begin(), std::move(m_listExterMsg));
        m_mtx.unlock();
    }
    return std::move(msg);
}
