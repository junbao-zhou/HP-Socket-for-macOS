/*
* Copyright: JessMA Open Source (ldcsaa@gmail.com)
*
* Author	: Bruce Liang
* Website	: http://www.jessma.org
* Project	: https://github.com/ldcsaa
* Blog		: http://www.cnblogs.com/ldcsaa
* Wiki		: http://www.oschina.net/p/hp-socket
* QQ Group	: 75375912, 44636872
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "IODispatcher.h"
#include "FuncHelper.h"

#include <signal.h>
#include <pthread.h>
#include "MessagePipe.h"
#include "TimerPipe.h"

BOOL CIODispatcher::Start(IIOHandler *pHandler, int iWorkerMaxEvents, int iWorkers, LLONG llTimerInterval)
{
	ASSERT_CHECK_EINVAL(pHandler && iWorkerMaxEvents >= 0 && iWorkers >= 0);
	CHECK_ERROR(!HasStarted(), ERROR_INVALID_STATE);

	if (iWorkerMaxEvents == 0)
		iWorkerMaxEvents = DEF_WORKER_MAX_EVENTS;
	if (iWorkers == 0)
		iWorkers = DEFAULT_WORKER_THREAD_COUNT;

	m_iMaxEvents = iWorkerMaxEvents;
	m_iWorkers = iWorkers;
	m_pHandler = pHandler;

	//使用kqueue替代epoll
	m_kque = kqueue();
	fcntl(m_kque, F_SETFL, O_NONBLOCK | FD_CLOEXEC);
	CHECK_ERROR_FD(m_kque);

	//使用计数Event替代epoll中的EFD_SEMAPHORE
	m_evCmd = MessagePipe::Create();
	SET_NONBLOCK_CLOEXEC(m_evCmd->GetReadFd());

	if (IS_INVALID_FD(m_evCmd->GetReadFd()))
		goto START_ERROR;

	if (!VERIFY(CtlFD(m_evCmd->GetReadFd(), EV_ADD | EV_CLEAR, EVFILT_READ, m_evCmd)))
		goto START_ERROR;

	m_evExit = new CCounterEvent<true>();

	if (IS_INVALID_FD(m_evExit->GetFD()))
		goto START_ERROR;

	if (!VERIFY(CtlFD(m_evExit->GetFD(), EV_ADD | EV_CLEAR, EVFILT_READ, m_evExit)))
		goto START_ERROR;

	if (llTimerInterval > 0)
	{
		struct kevent event;
		m_evTimer = GenerateNextTimerIdent();

		EV_SET(&event, m_evTimer, EVFILT_TIMER, EV_ADD, NOTE_USECONDS, llTimerInterval * int64_t(1000), &m_evTimer);
		if (kevent(m_kque, &event, 1, nullptr, 0, nullptr) < 0)
			goto START_ERROR;
	}

	sigset_t ss;
	sigemptyset(&ss);
	sigaddset(&ss, SIGPIPE);

	VERIFY_IS_NO_ERROR(pthread_sigmask(SIG_BLOCK, &ss, nullptr));

	m_pWorkers = make_unique<CWorkerThread[]>(m_iWorkers);

	for (int i = 0; i < m_iWorkers; i++)
	{
		if (!VERIFY(m_pWorkers[i].Start(this, &CIODispatcher::WorkerProc)))
			goto START_ERROR;
	}

	return TRUE;

START_ERROR:
	EXECUTE_RESTORE_ERROR(Stop(FALSE));
	return FALSE;
}

BOOL CIODispatcher::Stop(BOOL bCheck)
{
	if (bCheck)
		CHECK_ERROR(HasStarted(), ERROR_INVALID_STATE);

	BOOL isOK = TRUE;
	if (m_pWorkers)
	{
		if (!m_evExit->Set(m_iWorkers))
		{
			isOK &= FALSE;
		}

		for (int i = 0; i < m_iWorkers; i++)
			isOK &= m_pWorkers[i].Join();
	}

	if (!m_queue.IsEmpty())
	{
		TDispCommand *pCmd = nullptr;

		while (m_queue.PopFront(&pCmd))
			TDispCommand::Destruct(pCmd);

		VERIFY(m_queue.IsEmpty());
	}

	if (IS_VALID_FD(m_evExit->GetFD()))
	{
		SAFE_DELETE(m_evExit);
	}

	if (m_evCmd && IS_VALID_FD(m_evCmd->GetReadFd()))
	{
		SAFE_DELETE(m_evCmd);
	}

	if (m_evTimer)
	{
		struct kevent event;
		EV_SET(&event, m_evTimer, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
		if (kevent(m_kque, &event, 1, nullptr, 0, nullptr) < 0)
			isOK &= FALSE;
	}

	if (IS_VALID_FD(m_kque))
		isOK &= IS_NO_ERROR(close(m_kque));

	Reset();

	return isOK;
}

VOID CIODispatcher::Reset()
{
	m_iWorkers = 0;
	m_iMaxEvents = 0;
	m_pHandler = nullptr;
	m_pWorkers = nullptr;
	m_kque = INVALID_FD;
	m_evCmd = nullptr;
	m_evExit = nullptr;
	m_evTimer = INVALID_FD;
}

BOOL CIODispatcher::SendCommand(USHORT t, UINT_PTR wp, UINT_PTR lp)
{
	return SendCommand(TDispCommand::Construct(t, wp, lp));
}

BOOL CIODispatcher::SendCommand(TDispCommand *pCmd)
{
	m_queue.PushBack(pCmd);
	constexpr char cTmp = 0x01;
	return m_evCmd->Write(&cTmp, 1) > 0;
}

BOOL CIODispatcher::CtlFD(FD fd, int op, UINT mask, PVOID pv)
{
	struct kevent evt;
	EV_SET(&evt, fd, mask, op, NULL, NULL, pv);
	return kevent(m_kque, &evt, 1, 0, 0, 0) >= 0;
}

int CIODispatcher::WorkerProc(PVOID pv)
{
    m_pHandler->OnDispatchThreadStart(SELF_THREAD_ID);
	BOOL bRun = TRUE;
	unique_ptr<struct kevent[]> pEvents = make_unique<struct kevent[]>(m_iMaxEvents);

	while (bRun)
	{
		int rs = kevent(m_kque, NULL, NULL, pEvents.get(), m_iMaxEvents, NULL);
		if (rs <= TIMEOUT)
			ERROR_ABORT();

		for (int i = 0; i < rs; i++)
		{
			//触发的文件描述符
			uintptr_t fd = pEvents[i].ident;

			//触发的事件
			UINT events = pEvents[i].filter;

			//附加Custom数据
			PVOID ptr = pEvents[i].udata;

			//可能被设置EV_OOBAND(具有外带数据)，当被shutdown时此项为EV_EOF
			uint16_t flags = pEvents[i].flags;

			//socket error (if any) in fflags，当socket出现error(EVFILT_EXCEPT)会被设置
			uint32_t fflags = pEvents[i].fflags;

			//触发的byte字节数，当为listen套接字时为待接收的连接数量
			intptr_t count = pEvents[i].data;

			if (ptr == &m_evTimer)
				ProcessTimer(&count, events);
			else if (ptr == m_evCmd)
				ProcessCommand(events);
			else if (ptr == m_evExit)
				bRun = ProcessExit(events);
			else
				ProcessIo(ptr, events, flags);
		}
	}

    m_pHandler->OnDispatchThreadEnd(SELF_THREAD_ID);

	return 0;
}

BOOL CIODispatcher::ProcessCommand(UINT events)
{
	if (events == EVFILT_EXCEPT)
		ERROR_ABORT();

	if (events != EVFILT_READ)
		return FALSE;

	BOOL isOK = TRUE;

	char cTmp;
	int rs = m_evCmd->Read(&cTmp, 1);
	if (rs == 1)
	{
		ASSERT(cTmp > 0);

		TDispCommand *pCmd = nullptr;

		while (m_queue.PopFront(&pCmd))
		{
			m_pHandler->OnCommand(pCmd);
			TDispCommand::Destruct(pCmd);
		}
	}
	else
	{
		ASSERT(IS_WOULDBLOCK_ERROR());

		isOK = FALSE;
	}

	return isOK;
}

BOOL CIODispatcher::ProcessTimer(PVOID ptr, UINT events)
{
	if (events == EVFILT_EXCEPT)
		ERROR_ABORT();

	if (events != EVFILT_TIMER)
		return TRUE;

	BOOL isOK = TRUE;

	m_pHandler->OnTimer(*(intptr_t *)ptr);

	return isOK;
}

BOOL CIODispatcher::ProcessExit(UINT events)
{
	if (events == EVFILT_EXCEPT)
		ERROR_ABORT();

	if (events != EVFILT_READ)
		return TRUE;

	BOOL bRun = TRUE;
	int64_t v;

	int rs = m_evExit->Get(v);
	if (rs != 1)
		ASSERT(IS_WOULDBLOCK_ERROR());
	else
	{
		ASSERT(v == 1);
		bRun = FALSE;
	}

	return bRun;
}

BOOL CIODispatcher::ProcessIo(PVOID ptr, UINT events, uint16_t flags)
{
	if (!m_pHandler->OnBeforeProcessIo(ptr, events, flags))
		return FALSE;

	BOOL rs = DoProcessIo(ptr, events, flags);
	m_pHandler->OnAfterProcessIo(ptr, events, rs);

	return rs;
}

BOOL CIODispatcher::DoProcessIo(PVOID ptr, UINT events, uint16_t flags)
{
	if (events == EVFILT_EXCEPT || EV_ERROR == flags)
		return m_pHandler->OnError(ptr, events);
	if ((events == (UINT)EVFILT_READ && flags == EV_OOBAND) && !m_pHandler->OnReadyPrivilege(ptr, events))
		return FALSE;
	if (((events == (UINT)EVFILT_READ || events == (UINT)EVFILT_TIMER) && flags != EV_OOBAND) && !m_pHandler->OnReadyRead(ptr, events))
		return FALSE;
	if ((events == (UINT)EVFILT_WRITE) && !m_pHandler->OnReadyWrite(ptr, events))
		return FALSE;
	if ((events == (UINT)(EVFILT_USER)) && !m_pHandler->OnHungUp(ptr, events))
		return FALSE;

	return TRUE;
}

uint32_t CIODispatcher::AddTimer(LLONG llInterval, PVOID pv)
{
	if (llInterval > 0)
	{
		struct kevent event;
		uint32_t timerIdent = GenerateNextTimerIdent();

		EV_SET(&event, timerIdent, EVFILT_TIMER, EV_ADD, NOTE_USECONDS, llInterval * int64_t(1000), pv);
		if (kevent(m_kque, &event, 1, nullptr, 0, nullptr) < 0)
			goto LAST_GOTO;

		return timerIdent;
	}

	LAST_GOTO:
	return 0;
}

BOOL CIODispatcher::DelTimer(uint32_t fdTimer)
{
	BOOL isOK = FALSE;

	if (fdTimer > 0)
	{
		struct kevent event;
		EV_SET(&event, fdTimer, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
		kevent(m_kque, &event, 1, nullptr, 0, nullptr) < 0 ? isOK = FALSE : isOK = TRUE;
	}

	return isOK;
}
