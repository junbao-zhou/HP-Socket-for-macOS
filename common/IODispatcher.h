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

#pragma once

#include "GlobalDef.h"
#include "Singleton.h"
#include "RingBuffer.h"
#include "Thread.h"

#include "MessagePipe.h"
#include "Event.h"
#include "TimerPipe.h"
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

#include <memory>

using namespace std;

#define DISP_EVENT_FLAG_R			1
#define DISP_EVENT_FLAG_W			2
#define DISP_EVENT_FLAG_H			4
#define RETRIVE_EVENT_FLAG_R(evt)	((evt) == (EVFILT_READ) ? DISP_EVENT_FLAG_R : 0)
#define RETRIVE_EVENT_FLAG_W(evt)	((evt) == (EVFILT_WRITE) ? DISP_EVENT_FLAG_W : 0)
#define RETRIVE_EVENT_FLAG_RW(evt)	(RETRIVE_EVENT_FLAG_R(evt) | RETRIVE_EVENT_FLAG_W(evt))
#define RETRIVE_EVENT_FLAG_H(evt)	((evt) == (EVFILT_USER) ? DISP_EVENT_FLAG_H : 0)

// ------------------------------------------------------------------------------------------------------------------------------------------------------- //

struct TDispCommand
{
	USHORT	 type;
	UINT_PTR wParam;
	UINT_PTR lParam;

	static TDispCommand* Construct(USHORT t, UINT_PTR wp = 0, UINT_PTR lp = 0)
		{return new TDispCommand(t, wp, lp);}

	static VOID Destruct(TDispCommand* p)
		{if(p) delete p;}

private:
	TDispCommand(USHORT t, UINT_PTR wp = 0, UINT_PTR lp = 0)
	: type(t), wParam(wp), lParam(lp)
	{
	}

	~TDispCommand() = default;
};

// ------------------------------------------------------------------------------------------------------------------------------------------------------- //

class IIOHandler
{
public:

	virtual VOID OnCommand(TDispCommand* pCmd)						= 0;
	virtual VOID OnTimer(LLONG llExpirations)						= 0;
    virtual BOOL OnBeforeProcessIo(PVOID ptr, UINT events)			= 0;
    virtual VOID OnAfterProcessIo(PVOID ptr, UINT events, BOOL rs)	= 0;
    virtual BOOL OnReadyPrivilege(PVOID ptr, UINT events)			= 0;
	virtual VOID OnDispatchThreadEnd(THR_ID tid)					= 0;

    //未提供默认实现
    virtual BOOL OnReadyRead(PVOID ptr, UINT events)					= 0;
    virtual BOOL OnReadyWrite(PVOID ptr, UINT events)				= 0;
    virtual BOOL OnHungUp(PVOID ptr, UINT events)					= 0;
    virtual BOOL OnError(PVOID ptr, UINT events)						= 0;

public:
	virtual ~IIOHandler() = default;
};

class CIOHandler : public IIOHandler
{
public:
	virtual VOID OnCommand(TDispCommand* pCmd)						override {}
	virtual VOID OnTimer(LLONG llExpirations)						override {}

    virtual BOOL OnBeforeProcessIo(PVOID ptr, UINT events)			override {return TRUE;}
    virtual VOID OnAfterProcessIo(PVOID ptr, UINT events, BOOL rs)	override {}
    virtual BOOL OnReadyPrivilege(PVOID ptr, UINT events)			override {return TRUE;}
	virtual VOID OnDispatchThreadEnd(THR_ID tid)					override {}
};

// ------------------------------------------------------------------------------------------------------------------------------------------------------- //

class CIODispatcher
{
public:
	static const int DEF_WORKER_MAX_EVENTS	= 64;

	using CCommandQueue	= CCASQueue<TDispCommand>;
	using CWorkerThread	= CThread<CIODispatcher, VOID, int>;

public:
	BOOL Start(IIOHandler* pHandler, int iWorkerMaxEvents = DEF_WORKER_MAX_EVENTS, int iWorkers = 0, LLONG llTimerInterval = 0);
	BOOL Stop(BOOL bCheck = TRUE);

	BOOL SendCommand(TDispCommand* pCmd);
	BOOL SendCommand(USHORT t, UINT_PTR wp = 0, UINT_PTR lp = 0);

	template<class _List, typename = enable_if_t<is_same<remove_reference_t<typename _List::reference>, TDispCommand*>::value>>
	BOOL SendCommands(const _List& cmds)
	{
		size_t size = cmds.size();
		if(size == 0) return FALSE;

		for(auto it = cmds.begin(), end = cmds.end(); it != end; ++it)
			m_queue.PushBack(*it);

        char cTmp = 0x01;
        int rs = 0;
        for(int i = 0; i < size; ++i){
            rs = m_evCmd->Write(&cTmp, 1);
            ASSERT(rs);
        }

        return VERIFY_IS_NO_ERROR(rs);
	}

	BOOL CtlFD(FD fd, int op, UINT mask, PVOID pv);
    BOOL ProcessIo(PVOID ptr, UINT events, uint16_t flags);

private:
	int WorkerProc(PVOID pv = nullptr);
	BOOL ProcessExit(UINT events);
    BOOL ProcessTimer(PVOID ptr, UINT events);
	BOOL ProcessCommand(UINT events);
    BOOL DoProcessIo(PVOID ptr, UINT events, uint16_t flags);
	
	VOID Reset();

public:
	BOOL HasStarted()	{return m_pHandler && m_pWorkers;}
	const CWorkerThread* GetWorkerThreads() {return m_pWorkers.get();}

	CIODispatcher()		{Reset();}
	~CIODispatcher()	{if(HasStarted()) Stop();}

private:
    IIOHandler*                 m_pHandler;
    FD                          m_kque;
    MessagePipe*                m_evCmd;
    CCounterEvent<true>*		m_evExit;
    uint64_t                    m_evTimer;
    int                         m_iWorkers;
    int                         m_iMaxEvents;

	CCommandQueue				m_queue;
	unique_ptr<CWorkerThread[]>	m_pWorkers;
};
