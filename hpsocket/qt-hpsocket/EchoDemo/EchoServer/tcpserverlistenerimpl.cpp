#include "tcpserverlistenerimpl.h"
#include <QString>

EnHandleResult TcpServerListenerImpl::OnSend(ITcpServer *pSender, CONNID dwConnID, const BYTE *pData, int iLength)
{
    return HR_OK;
}

EnHandleResult TcpServerListenerImpl::OnReceive(ITcpServer *pSender, CONNID dwConnID, const BYTE *pData, int iLength)
{
    //响应
    if(!pSender->Send(dwConnID, pData, iLength)){
        return HR_ERROR;
    }

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

EnHandleResult TcpServerListenerImpl::OnClose(ITcpServer *pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
    return HR_OK;
}

std::list<QString> TcpServerListenerImpl::getExterMsg()
{
    std::list<QString> msg;
    if(m_mtx.try_lock()){
        msg.splice(msg.begin(), std::move(m_listExterMsg));
        m_mtx.unlock();
    }
    return std::move(msg);
}
