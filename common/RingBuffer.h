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
#include "STLHelper.h"
#include "FuncHelper.h"
#include "RWLock.h"
#include <list>
using namespace std;

#define CACHE_LINE		64
#define PACK_SIZE_OF(T)	(CACHE_LINE - sizeof(T) % CACHE_LINE)

#if !defined (__x86_64__)
	#pragma pack(push, 4)
#endif

// ------------------------------------------------------------------------------------------------------------- //
/*
    具体请看CRingCache2
    不同之处在于使用了一个索引系数数组
*/
template <class T, class index_type = DWORD, bool adjust_index = false> class CRingCache
{
public:

	enum EnGetResult {GR_FAIL = -1, GR_INVALID = 0, GR_VALID = 1};

	typedef T*									TPTR;
	typedef volatile T*							VTPTR;

	typedef unordered_set<index_type>			IndexSet;
	typedef typename IndexSet::const_iterator	IndexSetCI;
	typedef typename IndexSet::iterator			IndexSetI;

	static TPTR const E_EMPTY;
	static TPTR const E_LOCKED;
	static TPTR const E_MAX_STATUS;

public:

	static index_type& INDEX_INC(index_type& dwIndex)	{if(adjust_index) ++dwIndex; return dwIndex;}
	static index_type& INDEX_DEC(index_type& dwIndex)	{if(adjust_index) --dwIndex; return dwIndex;}

private:

	index_type& INDEX_V2R(index_type& dwIndex)	{dwIndex %= m_dwSize; if(dwIndex == 0) dwIndex = m_dwSize; return dwIndex;}
	VTPTR& INDEX_VAL(index_type dwIndex) {return *(m_pv + dwIndex);}

public:

	BOOL Put(TPTR pElement, index_type& dwIndex)
	{
		ASSERT(pElement != nullptr);

		if(!IsValid()) return FALSE;

		BOOL isOK = FALSE;

		while(true)
		{
			if(!HasSpace())
				break;

			DWORD dwCurSeq			= m_dwCurSeq;
			index_type dwCurIndex	= dwCurSeq % m_dwSize;
			VTPTR& pValue			= INDEX_VAL(dwCurIndex);

			if(pValue == E_EMPTY)
			{
				if(::InterlockedCompareExchangePointer((volatile PVOID*)&pValue, pElement, E_EMPTY) == E_EMPTY)
				{
					::InterlockedIncrement(&m_dwCount);
					::InterlockedCompareExchange(&m_dwCurSeq, dwCurSeq + 1, dwCurSeq);

					dwIndex = INDEX_INC(dwCurIndex);
					isOK	= TRUE;

					if(pElement != E_LOCKED)
						EmplaceIndex(dwIndex);

					break;
				}
			}

			::InterlockedCompareExchange(&m_dwCurSeq, dwCurSeq + 1, dwCurSeq);
		}

		return isOK;
	}

	EnGetResult GetEx(index_type dwIndex, TPTR* ppElement)
	{
		return Get(INDEX_V2R(dwIndex), ppElement);
	}

	EnGetResult Get(index_type dwIndex, TPTR* ppElement)
	{
		ASSERT(dwIndex <= m_dwSize);
		ASSERT(ppElement != nullptr);

		if(!IsValid() || INDEX_DEC(dwIndex) >= m_dwSize)
		{
			*ppElement = nullptr;
			return GR_FAIL;
		}

		*ppElement = (TPTR)INDEX_VAL(dwIndex);

		return IsValidElement(*ppElement) ? GR_VALID : GR_INVALID;
	}

	BOOL SetEx(index_type dwIndex, TPTR pElement, TPTR* ppOldElement = nullptr)
	{
		return Set(INDEX_V2R(dwIndex), pElement, ppOldElement);
	}

	BOOL Set(index_type dwIndex, TPTR pElement, TPTR* ppOldElement = nullptr)
	{
		TPTR pElement2 = nullptr;

		if(Get(dwIndex, &pElement2) == GR_FAIL)
			return FALSE;

		if(ppOldElement != nullptr)
			*ppOldElement = pElement2;

		if(pElement == pElement2)
			return FALSE;

		int f1 = 0;
		int f2 = 0;

		if(pElement == E_EMPTY)
		{
			if(pElement2 == E_LOCKED)
				f1 = -1;
			else
				f1 = f2 = -1;
		}
		else if(pElement == E_LOCKED)
		{
			if(pElement2 == E_EMPTY)
				f1 = 1;
			else
				f2 = -1;
		}
		else
		{
			if(pElement2 == E_EMPTY)
				f1 = f2 = 1;
			else if(pElement2 == E_LOCKED)
				f2 = 1;
		}

		BOOL bSetValueFirst		= (f1 + f2 >= 0);
		index_type dwOuterIndex	= dwIndex;

		INDEX_DEC(dwIndex);

		if(bSetValueFirst)	INDEX_VAL(dwIndex) = pElement;
		if(f1 > 0)			::InterlockedIncrement(&m_dwCount);
		if(f2 != 0)			(f2 > 0) ? EmplaceIndex(dwOuterIndex) : EraseIndex(dwOuterIndex);
		if(f1 < 0)			::InterlockedDecrement(&m_dwCount);
		if(!bSetValueFirst) INDEX_VAL(dwIndex) = pElement;

		ASSERT(Spaces() <= Size());

		return TRUE;
	}

	BOOL RemoveEx(index_type dwIndex, TPTR* ppElement = nullptr)
	{
		return Remove(INDEX_V2R(dwIndex), ppElement);
	}

	BOOL Remove(index_type dwIndex, TPTR* ppElement = nullptr)
	{
		return Set(dwIndex, E_EMPTY, ppElement);
	}

	BOOL AcquireLock(index_type& dwIndex)
	{
		return Put(E_LOCKED, dwIndex);
	}

	BOOL ReleaseLock(index_type dwIndex, TPTR pElement)
	{
		ASSERT(pElement == nullptr || IsValidElement(pElement));

		TPTR pElement2 = nullptr;
		Get(dwIndex, &pElement2);

		ASSERT(pElement2 == E_LOCKED);

		if(pElement2 != E_LOCKED)
			return FALSE;

		return Set(dwIndex, pElement);
	}

public:

	void Reset(DWORD dwSize = 0)
	{
		if(IsValid())
			Destroy();
		if(dwSize > 0)
			Create(dwSize);
	}

	BOOL GetAllElementIndexes(index_type ids[], DWORD& dwCount, BOOL bCopy = TRUE)
	{
		if(ids == nullptr || dwCount == 0)
		{
			dwCount = Elements();
			return FALSE;
		}

		IndexSet* pIndexes = nullptr;
		IndexSet indexes;

		if(bCopy)
			pIndexes = &CopyIndexes(indexes);
		else
			pIndexes = &m_indexes;

		BOOL isOK	 = FALSE;
		DWORD dwSize = (DWORD)pIndexes->size();

		if(dwSize > 0 && dwSize <= dwCount)
		{
			IndexSetCI it  = pIndexes->begin();
			IndexSetCI end = pIndexes->end();

			for(int i = 0; it != end; ++it, ++i)
				ids[i] = *it;

			isOK = TRUE;
		}

		dwCount = dwSize;
		return isOK;
	}

	unique_ptr<index_type[]> GetAllElementIndexes(DWORD& dwCount, BOOL bCopy = TRUE)
	{
		IndexSet* pIndexes = nullptr;
		IndexSet indexes;

		if(bCopy)
			pIndexes = &CopyIndexes(indexes);
		else
			pIndexes = &m_indexes;

		unique_ptr<index_type[]> ids;
		dwCount = (DWORD)pIndexes->size();

		if(dwCount > 0)
		{
			ids.reset(new index_type[dwCount]);

			IndexSetCI it  = pIndexes->begin();
			IndexSetCI end = pIndexes->end();

			for(int i = 0; it != end; ++it, ++i)
				ids[i] = *it;
		}

		return ids;
	}

	static BOOL IsValidElement(TPTR pElement) {return pElement > E_MAX_STATUS;}

	DWORD Size		()	{return m_dwSize;}
	DWORD Elements	()	{return (DWORD)m_indexes.size();}
	DWORD Spaces	()	{return m_dwSize - m_dwCount;}
	BOOL HasSpace	()	{return m_dwCount < m_dwSize;}
	BOOL IsEmpty	()	{return m_dwCount == 0;}
	BOOL IsValid	()	{return m_pv != nullptr;}

private:

	void Create(DWORD dwSize)
	{
		ASSERT(!IsValid() && dwSize > 0);

		m_dwCurSeq	= 0;
		m_dwCount	= 0;
		m_dwSize	= dwSize;
		m_pv		= (VTPTR*)malloc(m_dwSize * sizeof(TPTR));

		::ZeroMemory(m_pv, m_dwSize * sizeof(TPTR));
	}

	void Destroy()
	{
		ASSERT(IsValid());

		m_indexes.clear();
		free((void*)m_pv);

		m_pv		= nullptr;
		m_dwSize	= 0;
		m_dwCount	= 0;
		m_dwCurSeq	= 0;
	}

	IndexSet& CopyIndexes(IndexSet& indexes)
	{
		{
			CReadLock locallock(m_cs);
			indexes = m_indexes;
		}

		return indexes;
	}

	void EmplaceIndex(index_type dwIndex)
	{
		CWriteLock locallock(m_cs);
		m_indexes.emplace(dwIndex);
	}

	void EraseIndex(index_type dwIndex)
	{
		CWriteLock locallock(m_cs);
		m_indexes.erase(dwIndex);
	}

public:
	CRingCache	(DWORD dwSize = 0)
		: m_pv		(nullptr)
		, m_dwSize	(0)
		, m_dwCount	(0)
		, m_dwCurSeq(0)
	{
		Reset(dwSize);
	}

	~CRingCache()
	{
		Reset(0);
	}

private:
	CRingCache(const CRingCache&);
	CRingCache operator = (const CRingCache&);

private:
	DWORD				m_dwSize;
	VTPTR*				m_pv;
	char				pack1[PACK_SIZE_OF(VTPTR*)];
	volatile DWORD		m_dwCurSeq;
	char				pack2[PACK_SIZE_OF(DWORD)];
	volatile DWORD		m_dwCount;
	char				pack3[PACK_SIZE_OF(DWORD)];

	CSimpleRWLock		m_cs;
	IndexSet			m_indexes;
};

template <class T, class index_type, bool adjust_index> T* const CRingCache<T, index_type, adjust_index>::E_EMPTY		= (T*)0x00;
template <class T, class index_type, bool adjust_index> T* const CRingCache<T, index_type, adjust_index>::E_LOCKED		= (T*)0x01;
template <class T, class index_type, bool adjust_index> T* const CRingCache<T, index_type, adjust_index>::E_MAX_STATUS	= (T*)0x0F;

// ------------------------------------------------------------------------------------------------------------- //

template <class T, class index_type = DWORD, bool adjust_index = false> class CRingCache2
{
public:

        enum EnGetResult {
                            GR_FAIL = -1, //失败
                            GR_INVALID = 0, //无效
                            GR_VALID = 1 //有效
                         };

	typedef T*									TPTR;
	typedef volatile T*							VTPTR;

	typedef unordered_set<index_type>			IndexSet;
	typedef typename IndexSet::const_iterator	IndexSetCI;
	typedef typename IndexSet::iterator			IndexSetI;

    static TPTR const E_EMPTY; //位置为空 0x00
    static TPTR const E_LOCKED; //已经上锁（被占用）0x01
    static TPTR const E_MAX_STATUS; //0x0F
    static DWORD const MAX_SIZE; //最大大小受限与系统平台，若为x86则大小一定不为0xFFFFFFFF

public:
        //引索位置自增、自减，默认为不自增减
	static index_type& INDEX_INC(index_type& dwIndex)	{if(adjust_index) ++dwIndex; return dwIndex;}
	static index_type& INDEX_DEC(index_type& dwIndex)	{if(adjust_index) --dwIndex; return dwIndex;}

	index_type& INDEX_R2V(index_type& dwIndex)			{dwIndex += *(m_px + dwIndex) * m_dwSize; return dwIndex;}

	BOOL INDEX_V2R(index_type& dwIndex)
	{
		index_type m = dwIndex % m_dwSize;
		BYTE x		 = *(m_px + m);

		if(dwIndex / m_dwSize != x)
			return FALSE;

		dwIndex = m;
		return TRUE;
	}


private:

	VTPTR& INDEX_VAL(index_type dwIndex) {return *(m_pv + dwIndex);}

public:

	BOOL Put(TPTR pElement, index_type& dwIndex)
	{
		ASSERT(pElement != nullptr);

		if(!IsValid()) return FALSE;

		BOOL isOK = FALSE;

		while(true)
		{
            //具有可存入空间
			if(!HasSpace())
				break;

            //缓冲中当前序列下标
			DWORD dwCurSeq			= m_dwCurSeq;
            //取余获得索引
            index_type dwCurIndex	= dwCurSeq % m_dwSize;
            //得到相应索引在m_pv中的值
			VTPTR& pValue			= INDEX_VAL(dwCurIndex);

			if(pValue == E_EMPTY)
			{
				if(::InterlockedCompareExchangePointer(&pValue, pElement, E_EMPTY) == E_EMPTY)
				{
                    //增加数据计数
					::InterlockedIncrement(&m_dwCount);
                    //mark 1
					::InterlockedCompareExchange(&m_dwCurSeq, dwCurSeq + 1, dwCurSeq);

                    //设置出参
					dwIndex = INDEX_INC(INDEX_R2V(dwCurIndex));
					isOK	= TRUE;

					if(pElement != E_LOCKED)
                        EmplaceIndex(dwIndex); //记录此有效索引（已经添加到缓冲区中）

					break;
				}
			}
            //当设置失败向尝试后增一位进入下一个while判断
            //多线程情况下存在增加失败的事情，但并不影响之后的判断(因为m_dwCurSeq一定是增加的)
            //当再次判断失败后会继续while判断
            //如果这里执行成功，则以为着`mark 1`处增加失败，并不影响缓冲区的序列的正确性
			::InterlockedCompareExchange(&m_dwCurSeq, dwCurSeq + 1, dwCurSeq);
		}

		return isOK;
	}

	EnGetResult Get(index_type dwIndex, TPTR* ppElement, index_type* pdwRealIndex = nullptr)
	{
		ASSERT(ppElement != nullptr);

        if(!IsValid() || !INDEX_V2R(INDEX_DEC(dwIndex)))//INDEX_V2R使得传入的dwIndex变为实际索引值
		{
			*ppElement = nullptr;
			return GR_FAIL;
		}
        //此时dwIndex已经为实际索引
		*ppElement = (TPTR)INDEX_VAL(dwIndex);
        //设置出参
		if(pdwRealIndex) *pdwRealIndex = dwIndex;

        //判断指针的有效性
		return IsValidElement(*ppElement) ? GR_VALID : GR_INVALID;
	}

    /* 此方法不应该单独使用由其是涉及多线程时，需要其他函数配合 */
	BOOL Set(index_type dwIndex, TPTR pElement, TPTR* ppOldElement = nullptr, index_type* pdwRealIndex = nullptr)
	{
		TPTR pElement2 = nullptr;

        //在栈上分配空间，用完会被销毁。
		if(pdwRealIndex == nullptr)
			pdwRealIndex = (index_type*)alloca(sizeof(index_type));

        //获取缓冲区相应索引，并得到实际索引
		if(Get(dwIndex, &pElement2, pdwRealIndex) == GR_FAIL)
			return FALSE;

		if(ppOldElement != nullptr)
			*ppOldElement = pElement2;

        //同一个地址
		if(pElement == pElement2)
			return FALSE;

        int f1 = 0;
        int f2 = 0;
        //迷之算法，用以判断缓冲区中状态；
        /*
        参见：Remove、AcquireLock、ReleaseLock中的使用
            仔细想想可以瞬间开悟，这里的状态累加主要用以E_EMPTY、E_LOCKED、UserPtr之间的转换
            不需要完全明白，主要知道有什么作用即可
        */
		if(pElement == E_EMPTY)
		{
			if(pElement2 == E_LOCKED)
				f1 = -1;
			else
				f1 = f2 = -1;
		}
        else if(pElement == E_LOCKED)
        {
			if(pElement2 == E_EMPTY)
                f1 = 1;
            else
				f2 = -1;
		}
		else
        {
            if(pElement2 == E_EMPTY)
				f1 = f2 = 1;
            else if(pElement2 == E_LOCKED)
				f2 = 1;
		}

		BOOL bSetValueFirst		= (f1 + f2 >= 0);
		index_type dwRealIndex	= *pdwRealIndex;

        //此部分代码块主要由之前的状态转换决定，反正没有对总体了解之前懂了也是白懂
		if(bSetValueFirst)	INDEX_VAL(dwRealIndex) = pElement;
        //当(pElement)设置为E_LOCKED，并且缓冲区索引数据为E_EMPTY =>表示将要添加数据
        //或者 (pElement)设置值为数据指针，同时缓冲区索引数据为E_EMPTY才会增加计量大小=>正在添加数据
		if(f1 > 0)			::InterlockedIncrement(&m_dwCount);
		if(f2 != 0)			(f2 > 0) ? EmplaceIndex(dwIndex) : EraseIndex(dwIndex);
        //只有(pElement)设置(相应索引)为E_EMPTY时才会导致m_px[dwRealIndex]系数增加
		if(f1 < 0)			{::InterlockedDecrement(&m_dwCount); ++(*(m_px + dwRealIndex));}
        if(!bSetValueFirst) INDEX_VAL(dwRealIndex) = pElement;

		ASSERT(Spaces() <= Size());

		return TRUE;
	}

	BOOL Remove(index_type dwIndex, TPTR* ppElement = nullptr)
	{
		return Set(dwIndex, E_EMPTY, ppElement);
	}

	BOOL AcquireLock(index_type& dwIndex)
	{
		return Put(E_LOCKED, dwIndex);
	}

	BOOL ReleaseLock(index_type dwIndex, TPTR pElement)
	{
		ASSERT(pElement == nullptr || IsValidElement(pElement));

		TPTR pElement2 = nullptr;
		Get(dwIndex, &pElement2);

		ASSERT(pElement2 == E_LOCKED);

		if(pElement2 != E_LOCKED)
			return FALSE;

		return Set(dwIndex, pElement);
	}

public:

	void Reset(DWORD dwSize = 0)
	{
		if(IsValid())
			Destroy();
		if(dwSize > 0)
			Create(dwSize);
	}
	
	BOOL GetAllElementIndexes(index_type ids[], DWORD& dwCount, BOOL bCopy = TRUE)
	{
		if(ids == nullptr || dwCount == 0)
		{
			dwCount = Elements();
			return FALSE;
		}

		IndexSet* pIndexes = nullptr;
		IndexSet indexes;

		if(bCopy)
			pIndexes = &CopyIndexes(indexes);
		else
			pIndexes = &m_indexes;

		BOOL isOK	 = FALSE;
		DWORD dwSize = (DWORD)pIndexes->size();

		if(dwSize > 0 && dwSize <= dwCount)
		{
			IndexSetCI it  = pIndexes->begin();
			IndexSetCI end = pIndexes->end();

			for(int i = 0; it != end; ++it, ++i)
				ids[i] = *it;

			isOK = TRUE;
		}

		dwCount = dwSize;

		return isOK;
	}
	
	unique_ptr<index_type[]> GetAllElementIndexes(DWORD& dwCount, BOOL bCopy = TRUE)
	{
		IndexSet* pIndexes = nullptr;
		IndexSet indexes;

		if(bCopy)
			pIndexes = &CopyIndexes(indexes);
		else
			pIndexes = &m_indexes;

		unique_ptr<index_type[]> ids;
		dwCount = (DWORD)pIndexes->size();

		if(dwCount > 0)
		{
			ids.reset(new index_type[dwCount]);

			IndexSetCI it  = pIndexes->begin();
			IndexSetCI end = pIndexes->end();

			for(int i = 0; it != end; ++it, ++i)
				ids[i] = *it;
		}

		return ids;
	}

	static BOOL IsValidElement(TPTR pElement) {return pElement > E_MAX_STATUS;}

	DWORD Size		()	{return m_dwSize;}
	DWORD Elements	()	{return (DWORD)m_indexes.size();}
	DWORD Spaces	()	{return m_dwSize - m_dwCount;}
	BOOL HasSpace	()	{return m_dwCount < m_dwSize;}
	BOOL IsEmpty	()	{return m_dwCount == 0;}
	BOOL IsValid	()	{return m_pv != nullptr;}

private:

	void Create(DWORD dwSize)
	{
		ASSERT(!IsValid() && dwSize > 0 && dwSize <= MAX_SIZE);

		m_dwCurSeq	= 0;
		m_dwCount	= 0;
		m_dwSize	= dwSize;
		m_pv		= (VTPTR*)malloc(m_dwSize * sizeof(TPTR));
		m_px		= (BYTE*)malloc(m_dwSize * sizeof(BYTE));

		::ZeroMemory(m_pv, m_dwSize * sizeof(TPTR));
		::ZeroMemory(m_px, m_dwSize * sizeof(BYTE));
	}

	void Destroy()
	{
		ASSERT(IsValid());

		m_indexes.clear();
		free((void*)m_pv);
		free((void*)m_px);

		m_pv		= nullptr;
		m_px		= nullptr;
		m_dwSize	= 0;
		m_dwCount	= 0;
		m_dwCurSeq	= 0;
	}

	IndexSet& CopyIndexes(IndexSet& indexes)
	{
		{
			CReadLock locallock(m_cs);
			indexes = m_indexes;
		}

		return indexes;
	}

	void EmplaceIndex(index_type dwIndex)
	{
		CWriteLock locallock(m_cs);
		m_indexes.emplace(dwIndex);
	}

	void EraseIndex(index_type dwIndex)
	{
		CWriteLock locallock(m_cs);
		m_indexes.erase(dwIndex);
	}

public:
	CRingCache2	(DWORD dwSize = 0)
	: m_pv		(nullptr)
	, m_px		(nullptr)
	, m_dwSize	(0)
	, m_dwCount	(0)
	, m_dwCurSeq(0)
	{
		Reset(dwSize);
	}

	~CRingCache2()
	{
		Reset(0);
	}

	DECLARE_NO_COPY_CLASS(CRingCache2)

private:
    DWORD				m_dwSize; //缓冲区实际大小
    VTPTR*				m_pv; //指向缓冲区，由指针数组组成
    char				pack1[PACK_SIZE_OF(VTPTR*)]; //用以做字节对齐使用
    BYTE*				m_px; //系数，此数组用以与实际索引做乘运算出虚拟索引，长度与缓冲区恒等（同样是缓冲区状态的管理）
    char				pack2[PACK_SIZE_OF(BYTE*)]; //用以做字节对齐使用
    volatile DWORD		m_dwCurSeq; //缓冲中当前序列下标，只会增涨
    char				pack3[PACK_SIZE_OF(DWORD)]; //用以做字节对齐使用
    volatile DWORD		m_dwCount; //缓冲区中数量记录
    char				pack4[PACK_SIZE_OF(DWORD)]; //用以做字节对齐使用

    CSimpleRWLock		m_cs; //读写锁
    IndexSet			m_indexes; //无序的索引集合
};

template <class T, class index_type, bool adjust_index> T* const CRingCache2<T, index_type, adjust_index>::E_EMPTY		= (T*)0x00;
template <class T, class index_type, bool adjust_index> T* const CRingCache2<T, index_type, adjust_index>::E_LOCKED		= (T*)0x01;
template <class T, class index_type, bool adjust_index> T* const CRingCache2<T, index_type, adjust_index>::E_MAX_STATUS	= (T*)0x0F;

template <class T, class index_type, bool adjust_index> DWORD const CRingCache2<T, index_type, adjust_index>::MAX_SIZE	= 
#if !defined(__x86_64__)
																														  0x00FFFFFF
#else
																														  0xFFFFFFFF
#endif
																																	;
// ------------------------------------------------------------------------------------------------------------- //

/*
  CRingPool环形池：
    通过Get、Put两个索引维护序列关系
    序列中的最小单位为指针，而取出、存入的单位也为指针，所以不存在首、尾连续读的问题。
*/
template <class T> class CRingPool
{
private:

	typedef T*			TPTR;
	typedef volatile T*	VTPTR;

    static TPTR const E_EMPTY; //空位置
    static TPTR const E_LOCKED; //上锁中
    static TPTR const E_RELEASED; //释放
    static TPTR const E_OCCUPIED; //已占用的
	static TPTR const E_MAX_STATUS;

private:

	VTPTR& INDEX_VAL(DWORD dwIndex) {return *(m_pv + dwIndex);}

public:

	BOOL TryPut(TPTR pElement)
	{
		ASSERT(pElement != nullptr);

		if(!IsValid()) return FALSE;

		BOOL isOK = FALSE;

		while(true)
		{
			BOOL bOccupy = FALSE;
			DWORD seqPut = m_seqPut;

			if(!HasPutSpace(seqPut))
				break;

			DWORD dwIndex = seqPut % m_dwSize;
			VTPTR& pValue = INDEX_VAL(dwIndex);

            //
			if(pValue == E_RELEASED)
			{
                //尝试将已经释放的位置变为占有
				if(::InterlockedCompareExchangePointer(&pValue, E_OCCUPIED, E_RELEASED) == E_RELEASED)
                    bOccupy = TRUE; //占有中...
				else
                    //占有失败，可想此时失败的理由肯定是被使用
                    //同样的m_seqPut会被增加，若未增加也就是多个while判断，最坏返回False
                    continue;
			}
            //没有被占用或者为空时
			if(pValue == E_EMPTY || bOccupy)
			{
                //尝试增加当前序列值，若增加失败则表示被别的线程占有了，只需要下次循环就可以了
                //此时有一个疑难点，会不会出现bOccupy为TRUE，m_seqPut增加失败的情况？
                //不会出现的，当被占有时其他线程是无法对m_seqPut进行增步的
				if(::InterlockedCompareExchange(&m_seqPut, seqPut + 1, seqPut) == seqPut)
				{
					pValue	= pElement;
					isOK	= TRUE;

					break;
				}
			}
			else if(pValue == E_LOCKED)
				break;
		}

		return isOK;
	}

	BOOL TryGet(TPTR* ppElement)
	{
		ASSERT(ppElement != nullptr);

		if(!IsValid()) return FALSE;

		BOOL isOK = FALSE;

		while(true)
		{
			DWORD seqGet = m_seqGet;

			if(!HasGetSpace(seqGet))
				break;

			DWORD dwIndex = seqGet % m_dwSize;
			VTPTR& pValue = INDEX_VAL(dwIndex);

            //已经被上锁则返回False
			if(pValue == E_LOCKED)
				break;
            //当为E_EMPTY、E_RELEASED、E_OCCUPIED的状态的时，表明数据并没有实际添加到缓冲中
			else if(pValue != E_EMPTY && pValue != E_RELEASED && pValue != E_OCCUPIED)
			{
                //尝试将已经被获取的指针增步
				if(::InterlockedCompareExchange(&m_seqGet, seqGet + 1, seqGet) == seqGet)
				{
					ASSERT(pValue > E_MAX_STATUS);

					*(ppElement)	= (TPTR)pValue;
					pValue			= E_EMPTY;
					isOK			= TRUE;

					break;
				}
			}
		}

		return isOK;
	}

	BOOL TryLock(TPTR* ppElement, DWORD& dwIndex)
	{
		ASSERT(ppElement != nullptr);

		if(!IsValid()) return FALSE;

		BOOL isOK = FALSE;

		while(true)
		{
			DWORD seqGet = m_seqGet;

			if(!HasGetSpace(seqGet))
				break;

			dwIndex			= seqGet % m_dwSize;
			VTPTR& pValue	= INDEX_VAL(dwIndex);

			if(pValue == E_LOCKED)
				break;
			else if(pValue != E_EMPTY && pValue != E_RELEASED && pValue != E_OCCUPIED)
			{
				if(::InterlockedCompareExchange(&m_seqGet, seqGet + 1, seqGet) == seqGet)
				{
					ASSERT(pValue > E_MAX_STATUS);

					*(ppElement)	= (TPTR)pValue;
					pValue			= E_LOCKED;
					isOK			= TRUE;

					break;
				}
			}
		}

		return isOK;
	}

	BOOL ReleaseLock(TPTR pElement, DWORD dwIndex)
	{
		ASSERT(dwIndex < m_dwSize);
		ASSERT(pElement == nullptr || pElement > E_MAX_STATUS);

		if(!IsValid()) return FALSE;

		VTPTR& pValue = INDEX_VAL(dwIndex);
		VERIFY(pValue == E_LOCKED);

		if(pElement != nullptr)
		{
			for(DWORD i = 0; ; i++)
			{
				if(TryPut(pElement))
					break;

				DWORD dwPutIndex = m_seqPut % m_dwSize;

				if(dwIndex == dwPutIndex)
				{
					pValue = pElement;
					::InterlockedIncrement(&m_seqPut);

					return TRUE;
				}

				::YieldThread(i);
			}
		}

		pValue = E_RELEASED;

		return TRUE;
	}

public:

	void Reset(DWORD dwSize = 0)
	{
		if(IsValid())
			Destroy();
		if(dwSize > 0)
			Create(dwSize);
	}

	DWORD Size()		{return m_dwSize;}
	DWORD Elements()	{return m_seqPut - m_seqGet;}
	BOOL IsFull()		{return Elements() == Size();}
	BOOL IsEmpty()		{return Elements() == 0;}
	BOOL IsValid()		{return m_pv != nullptr;}

private:

	BOOL HasPutSpace(DWORD seqPut)
	{
		return ((int)(seqPut - m_seqGet) < (int)m_dwSize);
	}

	BOOL HasGetSpace(DWORD seqGet)
	{
		return ((int)(m_seqPut - seqGet) > 0);
	}

	void Create(DWORD dwSize)
	{
		ASSERT(!IsValid() && dwSize > 0);

		m_seqPut = 0;
		m_seqGet = 0;
		m_dwSize = dwSize;
		m_pv	 = (VTPTR*)malloc(m_dwSize * sizeof(TPTR));

		::ZeroMemory(m_pv, m_dwSize * sizeof(TPTR));
	}

	void Destroy()
	{
		ASSERT(IsValid());

		free((void*)m_pv);
		m_pv = nullptr;
		m_dwSize = 0;
		m_seqPut = 0;
		m_seqGet = 0;
	}

public:
	CRingPool(DWORD dwSize = 0)
	: m_pv(nullptr)
	, m_dwSize(0)
	, m_seqPut(0)
	, m_seqGet(0)
	{
		Reset(dwSize);
	}

	~CRingPool()
	{
		Reset(0);
	}

	DECLARE_NO_COPY_CLASS(CRingPool)

private:
        DWORD				m_dwSize; //缓冲区实际大小
        VTPTR*				m_pv; //指向缓冲区
        char				pack1[PACK_SIZE_OF(VTPTR*)]; //用以做字节对齐使用
        volatile DWORD		m_seqPut; //输入序列下标索引
        char				pack2[PACK_SIZE_OF(DWORD)]; //用以做字节对齐使用
        volatile DWORD		m_seqGet; //输出序列下标索引
        char				pack3[PACK_SIZE_OF(DWORD)]; //用以做字节对齐使用
};

template <class T> T* const CRingPool<T>::E_EMPTY		= (T*)0x00;
template <class T> T* const CRingPool<T>::E_LOCKED		= (T*)0x01;
template <class T> T* const CRingPool<T>::E_RELEASED	= (T*)0x02;
template <class T> T* const CRingPool<T>::E_OCCUPIED	= (T*)0x03;
template <class T> T* const CRingPool<T>::E_MAX_STATUS	= (T*)0x0F;

// ------------------------------------------------------------------------------------------------------------- //

/*
  无锁的线程安全队列
  基于CAS机制，内部使用Node链表
  队列并不维护内存管理，这就需要依赖外部调用者。
*/
template <class T> class CCASQueue
{
private:
	struct Node;
	typedef Node*			NPTR;
	typedef volatile Node*	VNPTR;
	typedef volatile UINT	VUINT;

	struct Node
	{
		T*		pValue;
		VNPTR	pNext;

		Node(T* val, NPTR next = nullptr)
		: pValue(val), pNext(next)
		{

		}
	};

public:

	void PushBack(T* pVal)
	{
		ASSERT(pVal != nullptr);

		VNPTR pTail	= nullptr;
        //创建队列最小单位
		NPTR pNode	= new Node(pVal);

		while(true)
		{
			pTail = m_pTail;
            //尝试将m_pTail指向替换成pNode
			if(::InterlockedCompareExchangePointer(&m_pTail, pNode, pTail) == pTail)
			{
                //此时pTail为上一个m_pTail
                //在此产生了疑问，若多个线程同时执行到在这里那会出现问题吗？
                //不会，因为每个线程中的pTail都代表着它视角下的前一个值，而这个值因为CAS操作下并没有出现出现竞争问题
                //也就是说看到的是不同的，建立了一种未被记录的连接状态
                //下一条执行语句就是进行记录这种连接状态
				pTail->pNext = pNode;
				break;
			}
		}

		::InterlockedIncrement(&m_iSize);
	}

	void UnsafePushBack(T* pVal)
	{
		ASSERT(pVal != nullptr);

		NPTR pNode		= new Node(pVal);
		m_pTail->pNext	= pNode;
		m_pTail			= pNode;
		
		::InterlockedIncrement(&m_iSize);
	}

	BOOL PopFront(T** ppVal)
	{
		ASSERT(ppVal != nullptr);

		if(IsEmpty())
			return FALSE;

		BOOL isOK	= FALSE;
		NPTR pHead	= nullptr;
		NPTR pNext	= nullptr;

		while(true)
        {
            //基于一个变量做为互斥锁，并不会导致阻塞等
            while(::InterlockedCompareExchange(&m_iLock, 1u, 0u) != 0)
				::YieldProcessor();

			pHead = (NPTR)m_pHead;
			pNext = (NPTR)pHead->pNext;

			if(pNext == nullptr)
			{
				m_iLock = 0;
				break;
			}

			*ppVal	= pNext->pValue;
			m_pHead	= pNext;

			m_iLock = 0;
			isOK	= TRUE;

			::InterlockedDecrement(&m_iSize);

			delete pHead;
			break;
		}

		return isOK;
	}

	BOOL UnsafePopFront(T** ppVal)
	{
		if(!UnsafePeekFront(ppVal))
			return FALSE;

		NPTR pHead	= (NPTR)m_pHead;
		NPTR pNext	= (NPTR)pHead->pNext;
		m_pHead		= pNext;

		::InterlockedDecrement(&m_iSize);

		delete pHead;
		return TRUE;
	}

	BOOL UnsafePeekFront(T** ppVal)
	{
		ASSERT(ppVal != nullptr);

		NPTR pNext = (NPTR)m_pHead->pNext;

		if(pNext == nullptr)
			return FALSE;

		*ppVal = pNext->pValue;

		return TRUE;
	}

    /*
        当Push为堆内存时，此处会导致内存泄漏
    */
    list<T*> Clear()
	{
        std::list<T*> lspVal;
		T* pVal;
        while(PopFront(&pVal)) lspVal.push_back(pVal);

        return lspVal;
	}

    list<T*> UnsafeClear()
	{
        std::list<T*> lspVal;
		T* pVal;
        while(UnsafePopFront(&pVal)) lspVal.push_back(pVal);
        return lspVal;
	}

public:

	UINT Size()		{return m_iSize;}
	BOOL IsEmpty()	{return m_iSize == 0;}

public:

	CCASQueue() : m_iLock(0), m_iSize(0)
	{
		NPTR pHead = new Node(nullptr);
		m_pHead = m_pTail = pHead;
	}

	~CCASQueue()
	{
		ASSERT(m_iLock == 0);
		ASSERT(m_iSize == 0);
		ASSERT(m_pHead != nullptr);
		ASSERT(m_pHead->pNext == nullptr);

        //队列释放时也未将Push的堆空间释放
		while(m_pHead != nullptr)
		{
			VNPTR pNode = m_pHead->pNext;

			delete m_pHead;
			m_pHead = pNode;
		}
	}

	DECLARE_NO_COPY_CLASS(CCASQueue)

private:
	VUINT	m_iLock;
	VUINT	m_iSize;
	VNPTR	m_pHead;
	VNPTR	m_pTail;
};

/*
  与CCASQueue的并无太大区别
  微小变化在于第一个节点中不使用T类型的指针，而使用T类型赋值
  从push_back也能看得出
  但依然不适合shared_ptr这类的，共享引用计数的智能模板指针，核心问题在其引用计数非原子类型上
 */
template <class T> class CCASSimpleQueue
{
private:
	struct Node;
	typedef Node*			NPTR;
	typedef volatile Node*	VNPTR;
	typedef volatile UINT	VUINT;

	struct Node
	{
		T		tValue;
		VNPTR	pNext;

		Node(T val, NPTR next = nullptr)
		: tValue(val), pNext(next)
		{

		}
	};

public:

	void PushBack(T tVal)
	{
		VNPTR pTail	= nullptr;
		NPTR pNode	= new Node(tVal);

		while(true)
		{
			pTail = m_pTail;

			if(::InterlockedCompareExchangePointer(&m_pTail, pNode, pTail) == pTail)
			{
				pTail->pNext = pNode;
				break;
			}
		}

		::InterlockedIncrement(&m_iSize);
	}

	void UnsafePushBack(T tVal)
	{
		NPTR pNode		= new Node(tVal);
		m_pTail->pNext	= pNode;
		m_pTail			= pNode;
		
		::InterlockedIncrement(&m_iSize);
	}

	BOOL PopFront(T* ptVal)
	{
		ASSERT(ptVal != nullptr);

		if(IsEmpty())
			return FALSE;

		BOOL isOK	= FALSE;
		NPTR pHead	= nullptr;
		NPTR pNext	= nullptr;

		while(true)
		{
			while(::InterlockedCompareExchange(&m_iLock, 1u, 0u) != 0)
				::YieldProcessor();

			pHead = (NPTR)m_pHead;
			pNext = (NPTR)pHead->pNext;

			if(pNext == nullptr)
			{
				m_iLock = 0;
				break;
			}

			*ptVal	= pNext->tValue;
			m_pHead	= pNext;

			m_iLock = 0;
			isOK	= TRUE;

			::InterlockedDecrement(&m_iSize);

			delete pHead;
			break;
		}

		return isOK;
	}

	BOOL UnsafePopFront(T* ptVal)
	{
		if(!UnsafePeekFront(ptVal))
			return FALSE;

		NPTR pHead	= (NPTR)m_pHead;
		NPTR pNext	= (NPTR)pHead->pNext;
		m_pHead		= pNext;

		::InterlockedDecrement(&m_iSize);

		delete pHead;
		return TRUE;
	}

	BOOL UnsafePeekFront(T* ptVal)
	{
		ASSERT(ptVal != nullptr);

		NPTR pNext = (NPTR)m_pHead->pNext;

		if(pNext == nullptr)
			return FALSE;

		*ptVal = pNext->tValue;

		return TRUE;
	}

	void Clear()
	{
		T tVal;
		while(PopFront(&tVal));
	}

	void UnsafeClear()
	{
		T tVal;
		while(UnsafePopFront(&tVal));
	}

public:

	UINT Size()		{return m_iSize;}
	BOOL IsEmpty()	{return m_iSize == 0;}

public:

	CCASSimpleQueue() : m_iLock(0), m_iSize(0)
	{
		NPTR pHead = new Node(0);
		m_pHead = m_pTail = pHead;
	}

	~CCASSimpleQueue()
	{
		ASSERT(m_iLock == 0);
		ASSERT(m_iSize == 0);
		ASSERT(m_pHead != nullptr);
		ASSERT(m_pHead->pNext == nullptr);

		while(m_pHead != nullptr)
		{
			VNPTR pNode = m_pHead->pNext;

			delete m_pHead;
			m_pHead = pNode;
		}
	}

	DECLARE_NO_COPY_CLASS(CCASSimpleQueue)

private:
	VUINT	m_iLock;
	VUINT	m_iSize;
	VNPTR	m_pHead;
	VNPTR	m_pTail;
};

#if !defined (__x86_64__)
	#pragma pack(pop)
#endif
