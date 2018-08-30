#ifndef CBUFFERPOOL_H
#define CBUFFERPOOL_H
#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"
#include "TBuffer.h"
/*
  数据结构组件使用实例
 */
class CBufferPool
{
    //基于环形池的FIFO
    using TBufferList	= CRingPool<TBuffer>;
    //基于CASQueue的无锁队列
    using TBufferQueue	= CCASQueue<TBuffer>;
    //基于环形缓冲
    using TBufferCache	= CRingCache<TBuffer, ULONG_PTR, true>;

public:
    void PutFreeBuffer(ULONG_PTR dwID);

    TBuffer* PutCacheBuffer(ULONG_PTR dwID);

    TBuffer* FindCacheBuffer(ULONG_PTR dwID);

    TBuffer* PickFreeBuffer(ULONG_PTR dwID);

    void PutFreeBuffer(TBuffer* pBuffer);

    void Prepare();

    void Clear();

private:
    void ReleaseGCBuffer(BOOL bForce = FALSE);
public:
    void SetItemCapacity	(DWORD dwItemCapacity)		{m_itPool.SetItemCapacity(dwItemCapacity);}
    void SetItemPoolSize	(DWORD dwItemPoolSize)		{m_itPool.SetPoolSize(dwItemPoolSize);}
    void SetItemPoolHold	(DWORD dwItemPoolHold)		{m_itPool.SetPoolHold(dwItemPoolHold);}

    void SetMaxCacheSize	(DWORD dwMaxCacheSize)		{m_dwMaxCacheSize	= dwMaxCacheSize;}
    void SetBufferLockTime	(DWORD dwBufferLockTime)	{m_dwBufferLockTime	= dwBufferLockTime;}
    void SetBufferPoolSize	(DWORD dwBufferPoolSize)	{m_dwBufferPoolSize	= dwBufferPoolSize;}
    void SetBufferPoolHold	(DWORD dwBufferPoolHold)	{m_dwBufferPoolHold	= dwBufferPoolHold;}

    DWORD GetItemCapacity	()							{return m_itPool.GetItemCapacity();}
    DWORD GetItemPoolSize	()							{return m_itPool.GetPoolSize();}
    DWORD GetItemPoolHold	()							{return m_itPool.GetPoolHold();}

    DWORD GetMaxCacheSize	()							{return m_dwMaxCacheSize;}
    DWORD GetBufferLockTime	()							{return m_dwBufferLockTime;}
    DWORD GetBufferPoolSize	()							{return m_dwBufferPoolSize;}
    DWORD GetBufferPoolHold	()							{return m_dwBufferPoolHold;}

    TBuffer* operator []	(ULONG_PTR dwID)			{return FindCacheBuffer(dwID);}

public:
    CBufferPool(DWORD dwPoolSize	 = DEFAULT_BUFFER_POOL_SIZE,
                DWORD dwPoolHold	 = DEFAULT_BUFFER_POOL_HOLD,
                DWORD dwLockTime	 = DEFAULT_BUFFER_LOCK_TIME,
                DWORD dwMaxCacheSize = DEFAULT_MAX_CACHE_SIZE);

    ~CBufferPool();

    DECLARE_NO_COPY_CLASS(CBufferPool)

public:
    CPrivateHeap&	GetPrivateHeap()	{return m_heap;}
    CItemPool&		GetItemPool()		{return m_itPool;}

public:
    static const DWORD DEFAULT_MAX_CACHE_SIZE;
    static const DWORD DEFAULT_ITEM_CAPACITY;
    static const DWORD DEFAULT_ITEM_POOL_SIZE;
    static const DWORD DEFAULT_ITEM_POOL_HOLD;
    static const DWORD DEFAULT_BUFFER_LOCK_TIME;
    static const DWORD DEFAULT_BUFFER_POOL_SIZE;
    static const DWORD DEFAULT_BUFFER_POOL_HOLD;

private:
    DWORD			m_dwMaxCacheSize;
    DWORD			m_dwBufferLockTime;
    DWORD			m_dwBufferPoolSize;
    DWORD			m_dwBufferPoolHold;

    CPrivateHeap	m_heap;
    CItemPool		m_itPool;

    //实际缓冲区
    TBufferCache	m_bfCache;

    //释放的Buffer列表
    TBufferList		m_lsFreeBuffer;

    //Buffer回收队列
    TBufferQueue	m_lsGCBuffer;
};


#endif // CBUFFERPOOL_H
