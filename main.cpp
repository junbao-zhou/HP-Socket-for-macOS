#include <QCoreApplication>
#include <iostream>
#include <memory>
#include "HttpServer.h"
#include "helper.h"
#include "common/StringT.h"


const LPCTSTR DEF_ADDRESS	= _T("0.0.0.0");
const USHORT HTTP_PORT		= 8080;
const USHORT HTTPS_PORT		= 8443;

class CHttpServerListenerImpl : public CHttpServerListener
{
private:
    virtual EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen);
    virtual EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient);
    virtual EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID);
    virtual EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
    virtual EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
    virtual EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode);
    virtual EnHandleResult OnShutdown(ITcpServer* pSender);

    virtual EnHttpParseResult OnMessageBegin(IHttpServer* pSender, CONNID dwConnID);
    virtual EnHttpParseResult OnRequestLine(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszMethod, LPCSTR lpszUrl);
    virtual EnHttpParseResult OnHeader(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszName, LPCSTR lpszValue);
    virtual EnHttpParseResult OnHeadersComplete(IHttpServer* pSender, CONNID dwConnID);
    virtual EnHttpParseResult OnBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
    virtual EnHttpParseResult OnChunkHeader(IHttpServer* pSender, CONNID dwConnID, int iLength);
    virtual EnHttpParseResult OnChunkComplete(IHttpServer* pSender, CONNID dwConnID);
    virtual EnHttpParseResult OnMessageComplete(IHttpServer* pSender, CONNID dwConnID);
    virtual EnHttpParseResult OnUpgrade(IHttpServer* pSender, CONNID dwConnID, EnHttpUpgradeType enUpgradeType);
    virtual EnHttpParseResult OnParseError(IHttpServer* pSender, CONNID dwConnID, int iErrorCode, LPCSTR lpszErrorDesc);

    virtual EnHandleResult OnWSMessageHeader(IHttpServer* pSender, CONNID dwConnID, BOOL bFinal, BYTE iReserved, BYTE iOperationCode, const BYTE lpszMask[4], ULONGLONG ullBodyLen);
    virtual EnHandleResult OnWSMessageBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength);
    virtual EnHandleResult OnWSMessageComplete(IHttpServer* pSender, CONNID dwConnID);

private:
    CStringA GetHeaderSummary(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszSep = "  ", int iSepCount = 0, BOOL bWithContentLength = TRUE);

public:
    CHttpServerListenerImpl(LPCTSTR lpszName)
    : m_strName	(lpszName)
    {
    }

public:
    CString m_strName;
};


// ------------------------------------------------------------------------------------------------------------- //

EnHandleResult CHttpServerListenerImpl::OnPrepareListen(ITcpServer* pSender, SOCKET soListen)
{
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;

    pSender->GetListenAddress(szAddress, iAddressLen, usPort);
    ::PostOnPrepareListen(szAddress, usPort, m_strName);

    return HR_OK;
}

EnHandleResult CHttpServerListenerImpl::OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient)
{
    BOOL bPass = TRUE;
    TCHAR szAddress[50];
    int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
    USHORT usPort;

    pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);

    ::PostOnAccept(dwConnID, szAddress, usPort, bPass, m_strName);

    return bPass ? HR_OK : HR_ERROR;
}

EnHandleResult CHttpServerListenerImpl::OnHandShake(ITcpServer* pSender, CONNID dwConnID)
{
    ::PostOnHandShake(dwConnID, m_strName);
    return HR_OK;
}

EnHandleResult CHttpServerListenerImpl::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
    ::PostOnReceive(dwConnID, pData, iLength, m_strName);

    if(pSender->Send(dwConnID, pData, iLength))
        return HR_OK;
    else
        return HR_ERROR;
}

EnHandleResult CHttpServerListenerImpl::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
    ::PostOnSend(dwConnID, pData, iLength, m_strName);
    return HR_OK;
}

EnHandleResult CHttpServerListenerImpl::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
    iErrorCode == SE_OK ? ::PostOnClose(dwConnID, m_strName)	:
    ::PostOnError(dwConnID, enOperation, iErrorCode, m_strName)	;

    CBufferPtr* pBuffer = nullptr;
    pSender->GetConnectionExtra(dwConnID, (PVOID*)&pBuffer);

    if(pBuffer) delete pBuffer;

    return HR_OK;
}

EnHandleResult CHttpServerListenerImpl::OnShutdown(ITcpServer* pSender)
{
    ::PostOnShutdown(m_strName);
    return HR_OK;
}

// ------------------------------------------------------------------------------------------------------------- //

EnHttpParseResult CHttpServerListenerImpl::OnMessageBegin(IHttpServer* pSender, CONNID dwConnID)
{
    ::PostOnMessageBegin(dwConnID, m_strName);
    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnRequestLine(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszMethod, LPCSTR lpszUrl)
{
    ::PostOnRequestLine(dwConnID, lpszMethod, pSender->GetUrlFieldSet(dwConnID), lpszUrl, m_strName);
    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnHeader(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszName, LPCSTR lpszValue)
{
    ::PostOnHeader(dwConnID, lpszName, lpszValue, m_strName);
    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnHeadersComplete(IHttpServer* pSender, CONNID dwConnID)
{
    CStringA strSummary = GetHeaderSummary(pSender, dwConnID, "    ", 0, TRUE);
    ::PostOnHeadersComplete(dwConnID, strSummary, m_strName);

    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
    ::PostOnBody(dwConnID, pData, iLength, m_strName);
    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnChunkHeader(IHttpServer* pSender, CONNID dwConnID, int iLength)
{
    ::PostOnChunkHeader(dwConnID, iLength, m_strName);
    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnChunkComplete(IHttpServer* pSender, CONNID dwConnID)
{
    ::PostOnChunkComplete(dwConnID, m_strName);
    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnMessageComplete(IHttpServer* pSender, CONNID dwConnID)
{
    ::PostOnMessageComplete(dwConnID, m_strName);

    if(pSender->IsUpgrade(dwConnID))
        return HPR_OK;

    CStringA strBody = GetHeaderSummary(pSender, dwConnID, "    ", 0, FALSE);
    int iBodyLength  = strBody.GetLength();
    BOOL bSkipBody	 = FALSE;

    if(strcmp(pSender->GetMethod(dwConnID), HTTP_METHOD_HEAD) == 0)
        bSkipBody = TRUE;

    CStringA strContentLength;
    strContentLength.Format("%u", iBodyLength);

    DWORD dwSeq				= 1;
    LPCSTR lpszReqSequence	= nullptr;
    CStringA strSeq;

    if(pSender->GetCookie(dwConnID, "__reqSequence_1", &lpszReqSequence))
        dwSeq += atoi(lpszReqSequence);

    strSeq.Format("%u", dwSeq);

    CStringA strSeqCookie1 = CCookie::ToString("__reqSequence_1", strSeq, nullptr, nullptr, -1, TRUE, TRUE, CCookie::SS_LAX);

    dwSeq			= 1;
    lpszReqSequence	= nullptr;

    if(pSender->GetCookie(dwConnID, "__reqSequence_2", &lpszReqSequence))
        dwSeq += atoi(lpszReqSequence);

    strSeq.Format("%u", dwSeq);

    CStringA strSeqCookie2 = CCookie::ToString("__reqSequence_2", strSeq, nullptr, "/", 300, FALSE, FALSE, CCookie::SS_NONE);

    THeader header[] = {{"Content-Type", "text/plain"}, {"Content-Length", strContentLength}, {"Set-Cookie", strSeqCookie1}, {"Set-Cookie", strSeqCookie2}};
    int iHeaderCount = sizeof(header) / sizeof(THeader);

    if(bSkipBody)
    {
        strBody.Empty();
        iBodyLength = 0;
    }

    pSender->SendResponse(	dwConnID,
                            HSC_OK,
                            "HP Http Server OK",
                            header, iHeaderCount,
                            (const BYTE*)(LPCSTR)strBody,
                            iBodyLength);

    if(!pSender->IsKeepAlive(dwConnID))
        pSender->Release(dwConnID);

    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnUpgrade(IHttpServer* pSender, CONNID dwConnID, EnHttpUpgradeType enUpgradeType)
{
    ::PostOnUpgrade(dwConnID, enUpgradeType, m_strName);

    if(enUpgradeType == HUT_HTTP_TUNNEL)
    {
        pSender->SendResponse(dwConnID, HSC_OK, "Connection Established");
    }
    else if(enUpgradeType == HUT_WEB_SOCKET)
    {
        int iHeaderCount = 2;
        THeader header[] = {{"Connection", UPGRADE_HEADER},
                            {UPGRADE_HEADER, WEB_SOCKET_HEADER_VALUE},
                            {nullptr, nullptr},
                            {nullptr, nullptr}};

        LPCSTR lpszAccept = nullptr;

        if(!pSender->GetHeader(dwConnID, "Sec-WebSocket-Key", &lpszAccept))
            return HPR_ERROR;

        CStringA strAccept;
        ::MakeSecWebSocketAccept(lpszAccept, strAccept);

        header[2].name	= "Sec-WebSocket-Accept";
        header[2].value	= strAccept;
        ++iHeaderCount;

        LPCSTR lpszProtocol = nullptr;

        if(pSender->GetHeader(dwConnID, "Sec-WebSocket-Protocol", &lpszProtocol))
        {
            int i = 0;
            CStringA strProtocol(lpszProtocol);
            CStringA strFirst = strProtocol.Tokenize(", ", i);

            if(!strFirst.IsEmpty())
            {
                header[3].name	= "Sec-WebSocket-Protocol";
                header[3].value	= strFirst;
                ++iHeaderCount;
            }
        }

        pSender->SendResponse(dwConnID, HSC_SWITCHING_PROTOCOLS, nullptr, header, iHeaderCount);
        pSender->SetConnectionExtra(dwConnID, new CBufferPtr);
    }
    else
        ASSERT(FALSE);

    return HPR_OK;
}

EnHttpParseResult CHttpServerListenerImpl::OnParseError(IHttpServer* pSender, CONNID dwConnID, int iErrorCode, LPCSTR lpszErrorDesc)
{
    ::PostOnParseError(dwConnID, iErrorCode, lpszErrorDesc, m_strName);
    return HPR_OK;
}

// ------------------------------------------------------------------------------------------------------------- //

EnHandleResult CHttpServerListenerImpl::OnWSMessageHeader(IHttpServer* pSender, CONNID dwConnID, BOOL bFinal, BYTE iReserved, BYTE iOperationCode, const BYTE lpszMask[4], ULONGLONG ullBodyLen)
{
    ::PostOnWSMessageHeader(dwConnID, bFinal, iReserved, iOperationCode, lpszMask, ullBodyLen, m_strName);

    return HR_OK;
}

EnHandleResult CHttpServerListenerImpl::OnWSMessageBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
    ::PostOnWSMessageBody(dwConnID, pData, iLength, m_strName);

    CBufferPtr* pBuffer = nullptr;
    pSender->GetConnectionExtra(dwConnID, (PVOID*)&pBuffer);
    VERIFY(pBuffer);

    pBuffer->Cat(pData, iLength);

    return HR_OK;
}

EnHandleResult CHttpServerListenerImpl::OnWSMessageComplete(IHttpServer* pSender, CONNID dwConnID)
{
    ::PostOnWSMessageComplete(dwConnID, m_strName);

    CBufferPtr* pBuffer = nullptr;
    pSender->GetConnectionExtra(dwConnID, (PVOID*)&pBuffer);
    VERIFY(pBuffer);

    BOOL bFinal;
    BYTE iReserved, iOperationCode;

    VERIFY(pSender->GetWSMessageState(dwConnID, &bFinal, &iReserved, &iOperationCode, nullptr, nullptr, nullptr));

    pSender->SendWSMessage(dwConnID, bFinal, iReserved, iOperationCode, nullptr, pBuffer->Ptr(), (int)pBuffer->Size());
    pBuffer->Free();

    if(iOperationCode == 0x8)
        pSender->Disconnect(dwConnID);

    return HR_OK;
}

// ------------------------------------------------------------------------------------------------------------- //

CStringA CHttpServerListenerImpl::GetHeaderSummary(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszSep, int iSepCount, BOOL bWithContentLength)
{
    CStringA SEP1;

    for(int i = 0; i < iSepCount; i++)
        SEP1 += lpszSep;

    CStringA SEP2(SEP1);
    SEP2 += lpszSep;

    CStringA strResult;

    USHORT usUrlFieldSet = pSender->GetUrlFieldSet(dwConnID);

    strResult.AppendFormat("%s[URL Fields]%s", SEP1.c_str(), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "SCHEMA", pSender->GetUrlField(dwConnID, HUF_SCHEMA), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "HOST", pSender->GetUrlField(dwConnID, HUF_HOST), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "PORT", pSender->GetUrlField(dwConnID, HUF_PORT), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "PATH", pSender->GetUrlField(dwConnID, HUF_PATH), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "QUERY", pSender->GetUrlField(dwConnID, HUF_QUERY), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "FRAGMENT", pSender->GetUrlField(dwConnID, HUF_FRAGMENT), CRLF);
    strResult.AppendFormat("%s%8s: %s%s", SEP2.c_str(), "USERINFO", pSender->GetUrlField(dwConnID, HUF_USERINFO), CRLF);

    DWORD dwHeaderCount = 0;
    pSender->GetAllHeaders(dwConnID, nullptr, dwHeaderCount);

    strResult.AppendFormat("%s[Request Headers]%s", SEP1.c_str(), CRLF);

    if(dwHeaderCount == 0)
        strResult.AppendFormat("%s(no header)%s", SEP2.c_str(), CRLF);
    else
    {
        unique_ptr<THeader[]> headers(new THeader[dwHeaderCount]);
        VERIFY(pSender->GetAllHeaders(dwConnID, headers.get(), dwHeaderCount));

        for(DWORD i = 0; i < dwHeaderCount; i++)
            strResult.AppendFormat("%s%s: %s%s", SEP2.c_str(), headers[i].name, headers[i].value, CRLF);
    }

    DWORD dwCookieCount = 0;
    pSender->GetAllCookies(dwConnID, nullptr, dwCookieCount);

    strResult.AppendFormat("%s[Cookies]%s", SEP1.c_str(), CRLF);

    if(dwCookieCount == 0)
        strResult.AppendFormat("%s(no cookie)%s", SEP2.c_str(), CRLF);
    else
    {
        unique_ptr<TCookie[]> cookies(new TCookie[dwCookieCount]);
        VERIFY(pSender->GetAllCookies(dwConnID, cookies.get(), dwCookieCount));

        for(DWORD i = 0; i < dwCookieCount; i++)
            strResult.AppendFormat("%s%s: %s%s", SEP2.c_str(), cookies[i].name, cookies[i].value, CRLF);
    }

    CStringA strVersion;
    ::HttpVersionToString((EnHttpVersion)pSender->GetVersion(dwConnID), strVersion);
    EnHttpUpgradeType enUpgType	= pSender->GetUpgradeType(dwConnID);
    LPCSTR lpszUpgrade			= enUpgType != HUT_NONE ? "true" : "false";
    LPCSTR lpszKeepAlive		= pSender->IsKeepAlive(dwConnID) ? "true" : "false";

    strResult.AppendFormat("%s[Basic Info]%s", SEP1.c_str(), CRLF);
    strResult.AppendFormat("%s%13s: %s%s", SEP2.c_str(), "Version", strVersion.c_str(), CRLF);
    strResult.AppendFormat("%s%13s: %s%s", SEP2.c_str(), "Method", pSender->GetMethod(dwConnID), CRLF);
    strResult.AppendFormat("%s%13s: %s%s", SEP2.c_str(), "IsUpgrade", lpszUpgrade, CRLF);
    if(enUpgType != HUT_NONE)
        strResult.AppendFormat("%s%13s: %d%s", SEP2.c_str(), "UpgradeType", enUpgType, CRLF);
    strResult.AppendFormat("%s%13s: %s%s", SEP2.c_str(), "IsKeepAlive", lpszKeepAlive, CRLF);
    if(bWithContentLength)
        strResult.AppendFormat("%s%13s: %lld%s", SEP2.c_str(), "ContentLength", pSender->GetContentLength(dwConnID), CRLF);
    strResult.AppendFormat("%s%13s: %s%s", SEP2.c_str(), "ContentType", pSender->GetContentType(dwConnID), CRLF);

    return strResult;
}

const CString SPECIAL_SERVER_NAME	= _T("hpsocket.org");
int SPECIAL_SERVER_INDEX			= -1;

int CALLBACK SIN_ServerNameCallback(LPCTSTR lpszServerName)
{
    if(::IsIPAddress(lpszServerName))
        return 0;

    int len  = lstrlen(lpszServerName);
    int diff = len - SPECIAL_SERVER_NAME.GetLength();

    if(diff < 0)
        return 0;

    if(SPECIAL_SERVER_NAME.CompareNoCase(lpszServerName + diff) != 0)
        return 0;

    return SPECIAL_SERVER_INDEX;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CHttpServerListenerImpl listenerImpl(HTTP_NAME);

    CHttpServerListenerImpl listenerImplSSL(HTTPS_NAME);

    CHttpServer httpserver(&listenerImpl);
    CHttpsServer s_https_server(&listenerImplSSL);
    if(s_https_server.SetupSSLContext(SSL_VM_NONE, g_s_lpszPemCertFile, g_s_lpszPemKeyFile, g_s_lpszKeyPasswod, g_s_lpszCAPemCertFileOrPath, SIN_ServerNameCallback))
            SPECIAL_SERVER_INDEX = s_https_server.AddSSLContext(SSL_VM_NONE, g_s_lpszPemCertFile2, g_s_lpszPemKeyFile2, g_s_lpszKeyPasswod2, g_s_lpszCAPemCertFileOrPath2);
    else
    {
        httpserver.Stop();
        ::LogServerStartFail(::GetLastError(), _T("initialize SSL env fail"));
        return EXIT_CODE_CONFIG;
    }

    if(httpserver.Start(DEF_ADDRESS, HTTP_PORT))
    {
        ::LogServerStart(DEF_ADDRESS, HTTP_PORT, HTTP_NAME);
        if(!s_https_server.Start(DEF_ADDRESS, HTTPS_PORT)){
            httpserver.Stop();
            printf("bye bye~\n");
        }
    }

    std::cin.get();
    return a.exec();
}
