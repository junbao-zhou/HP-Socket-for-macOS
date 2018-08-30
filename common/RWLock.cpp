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

#include "RWLock.h"

#if defined(_USE_MUTEX_RW_LOCK)
CMutexRWLock::CMutexRWLock()
	: m_nActive			(0)
	, m_nReadCount		(0)
	, m_dwWriterTID		(0)
{

}

CMutexRWLock::~CMutexRWLock()
{
	ASSERT(m_nActive	 == 0);
	ASSERT(m_nReadCount	 == 0);
	ASSERT(m_dwWriterTID == 0);
}

VOID CMutexRWLock::WaitToRead()
{
	BOOL bWait = FALSE;

	{
		CSpinLock locallock(m_cs);

		if(m_nActive > 0)
			++m_nActive;
		else if(m_nActive == 0)
		{
			if(m_mtx.try_lock_shared())
			{
				++m_nReadCount;
				++m_nActive;
			}
			else
				bWait = TRUE;
		}
		else if(!IsOwner())
			bWait = TRUE;
	}

	if(bWait)
	{
		m_mtx.lock_shared();

		CSpinLock locallock(m_cs);

		{
			++m_nReadCount;
			++m_nActive;
		}
	}
}

VOID CMutexRWLock::WaitToWrite()
{
	BOOL bWait = FALSE;

	{
		CSpinLock locallock(m_cs);

		if(m_nActive > 0)
			bWait = TRUE;
		else if(m_nActive == 0)
		{
			if(m_mtx.try_lock())
			{
				SetOwner();
				--m_nActive;
			}
			else
				bWait = TRUE;
		}
		else
		{
			if(IsOwner())
				--m_nActive;
			else
				bWait = TRUE;
		}
	}

	if(bWait)
	{
		m_mtx.lock();

		SetOwner();
		--m_nActive;
	}
}

VOID CMutexRWLock::ReadDone()
{
	ASSERT(m_nActive != 0);

	if(m_nActive > 0)
	{
		ASSERT(m_nReadCount > 0);

		CSpinLock locallock(m_cs);

		if(--m_nActive == 0)
		{
			for(; m_nReadCount > 0; --m_nReadCount)
				m_mtx.unlock_shared();
		}
	}
	else
		ASSERT(IsOwner());
}

VOID CMutexRWLock::WriteDone()
{
	ASSERT(IsOwner());
	ASSERT(m_nActive < 0);

	CSpinLock locallock(m_cs);

	if(++m_nActive == 0)
	{
		DetachOwner();
		m_mtx.unlock();
	}		
}
#endif

CSEMRWLock::CSEMRWLock()
	: m_nWaitingReaders	(0)
	, m_nWaitingWriters	(0)
	, m_nActive			(0)
	, m_dwWriterTID		(0)
{

}

CSEMRWLock::~CSEMRWLock()
{
	ASSERT(m_nActive	 == 0);
	ASSERT(m_dwWriterTID == 0);
}

VOID CSEMRWLock::WaitToRead()
{
	BOOL bWait = FALSE;

	{
		CSpinLock locallock(m_cs);

        //m_nActive > 0表示读 < 0表示写 == 0则表示无状态
        //若现在为读状态，可以重入
		if(m_nActive > 0)
			++m_nActive;
        else if(m_nActive == 0) //不存在读写状态
        {
            //判断等待写状态事件，不存在
			if(m_nWaitingWriters == 0)
				++m_nActive;
			else
			{
                //等待读
				++m_nWaitingReaders;
				bWait = TRUE;
			}
		}
		else
		{
            //不是权柄相应的线程
			if(!IsOwner())
			{
				++m_nWaitingReaders;
				bWait = TRUE;
			}
		}
	}

	if(bWait)
	{
		m_smRead.Wait();
	}
}

VOID CSEMRWLock::WaitToWrite()
{
	BOOL bWait = FALSE;

	{
		CSpinLock locallock(m_cs);

		if(m_nActive > 0)
		{
			++m_nWaitingWriters;
			bWait = TRUE;
		}
		else if(m_nActive == 0)
		{
			--m_nActive;
            //直接设置权柄此时的得主
			SetOwner();
		}
		else
		{
            //判断权柄线程，可以重入
			if(IsOwner())
				--m_nActive;
			else
			{
                //不是权柄相应的线程
				++m_nWaitingWriters;
				bWait = TRUE;
			}
		}
	}

	if(bWait)
	{
		m_smWrite.Wait();
		SetOwner();
	}
}

/*
  读锁定结束
  只会在读加锁开始之后使用
*/
VOID CSEMRWLock::ReadDone()
{
	ASSERT(m_nActive != 0);

	INT iFlag = 0;

	if(m_nActive > 0)
	{
		CSpinLock locallock(m_cs);

		if(--m_nActive == 0)
			iFlag = Done();
	}
    else//判断是否在读加锁后使用
		ASSERT(IsOwner());

	Notify(iFlag);
}

/*
  写锁定结束
  只会在写加锁开始之后使用
*/
VOID CSEMRWLock::WriteDone()
{
	ASSERT(IsOwner());
	ASSERT(m_nActive < 0);

	INT iFlag = 0;

	{
		CSpinLock locallock(m_cs);

        // 只会为 -1, 0, 1
		if(++m_nActive == 0)
		{
			DetachOwner();
			iFlag = Done();
		}
	}

	Notify(iFlag);
}

INT CSEMRWLock::Done()
{
	ASSERT(m_nActive	 == 0);
	ASSERT(m_dwWriterTID == 0);

	if(m_nWaitingWriters > 0)
	{
		--m_nActive;
		--m_nWaitingWriters;

        //写锁的唤醒
		return -1;
	}
	else if(m_nWaitingReaders > 0)
	{
		m_nActive			= m_nWaitingReaders;
		m_nWaitingReaders	= 0;
		
        //读锁的唤醒
		return 1;
	}

	return 0;
}
