#include "CBufferPool.h"


const DWORD CBufferPool::DEFAULT_MAX_CACHE_SIZE		= 0;
const DWORD CBufferPool::DEFAULT_ITEM_CAPACITY		= CItemPool::DEFAULT_ITEM_CAPACITY;
const DWORD CBufferPool::DEFAULT_ITEM_POOL_SIZE		= CItemPool::DEFAULT_POOL_SIZE;
const DWORD CBufferPool::DEFAULT_ITEM_POOL_HOLD		= CItemPool::DEFAULT_POOL_HOLD;
const DWORD CBufferPool::DEFAULT_BUFFER_LOCK_TIME	= 15 * 1000;
const DWORD CBufferPool::DEFAULT_BUFFER_POOL_SIZE	= 150;
const DWORD CBufferPool::DEFAULT_BUFFER_POOL_HOLD	= 600;


void CBufferPool::PutFreeBuffer(ULONG_PTR dwID){
    ASSERT(dwID != 0);

    TBuffer* pBuffer = FindCacheBuffer(dwID);

    if(pBuffer != nullptr)
        PutFreeBuffer(pBuffer);
}

TBuffer* CBufferPool::PutCacheBuffer(ULONG_PTR dwID){
    ASSERT(dwID != 0);

    TBuffer* pBuffer = PickFreeBuffer(dwID);
    m_bfCache.SetEx(dwID, pBuffer);

    return pBuffer;
}

TBuffer* CBufferPool::FindCacheBuffer(ULONG_PTR dwID){
    ASSERT(dwID != 0);

    TBuffer* pBuffer = nullptr;

    if(m_bfCache.GetEx(dwID, &pBuffer) != TBufferCache::GR_VALID)
        pBuffer = nullptr;

    return pBuffer;
}

TBuffer* CBufferPool::PickFreeBuffer(ULONG_PTR dwID){
    ASSERT( dwID != 0);

    DWORD dwIndex;
    TBuffer* pBuffer = nullptr;

    if(m_lsFreeBuffer.TryLock(&pBuffer, dwIndex))
    {
        if(::GetTimeGap32(pBuffer->freeTime) >= m_dwBufferLockTime)
            VERIFY(m_lsFreeBuffer.ReleaseLock(nullptr, dwIndex));
        else
        {
            VERIFY(m_lsFreeBuffer.ReleaseLock(pBuffer, dwIndex));
            pBuffer = nullptr;
        }
    }

    if(pBuffer)	pBuffer->id	= dwID;
    else		pBuffer		= TBuffer::Construct(*this, dwID);

    ASSERT(pBuffer);
    return pBuffer;
}

void CBufferPool::PutFreeBuffer(TBuffer* pBuffer){
    ASSERT(pBuffer != nullptr);

    if(!pBuffer->IsValid())
        return;

    m_bfCache.RemoveEx(pBuffer->ID());

    BOOL bOK = FALSE;

    {
        CCriSecLock locallock(pBuffer->cs);

        if(pBuffer->IsValid())
        {
            pBuffer->Reset();
            bOK = TRUE;
        }
    }

    if(bOK)
    {
        m_itPool.PutFreeItem(pBuffer->items);

        if(!m_lsFreeBuffer.TryPut(pBuffer))
        {
            m_lsGCBuffer.PushBack(pBuffer);

            if(m_lsGCBuffer.Size() > m_dwBufferPoolSize)
                ReleaseGCBuffer();
        }
    }
}

void CBufferPool::Prepare(){
    m_itPool.Prepare();

    m_bfCache.Reset(m_dwMaxCacheSize);
    m_lsFreeBuffer.Reset(m_dwBufferPoolHold);
}

void CBufferPool::Clear(){
    DWORD size = 0;
    unique_ptr<ULONG_PTR[]> ids	= m_bfCache.GetAllElementIndexes(size, FALSE);

    for(DWORD i = 0; i < size; i++)
    {
        TBuffer* pBuffer = FindCacheBuffer(ids[i]);
        if(pBuffer) TBuffer::Destruct(pBuffer);
    }

    m_bfCache.Reset();

    TBuffer* pBuffer = nullptr;

    while(m_lsFreeBuffer.TryGet(&pBuffer))
        TBuffer::Destruct(pBuffer);

    VERIFY(m_lsFreeBuffer.IsEmpty());
    m_lsFreeBuffer.Reset();

    ReleaseGCBuffer(TRUE);
    VERIFY(m_lsGCBuffer.IsEmpty());

    m_itPool.Clear();
    m_heap.Reset();
}

void CBufferPool::ReleaseGCBuffer(BOOL bForce){
    TBuffer* pBuffer = nullptr;
    DWORD now		 = ::TimeGetTime();

    while(m_lsGCBuffer.PopFront(&pBuffer))
    {
        if(bForce || (int)(now - pBuffer->freeTime) >= (int)m_dwBufferLockTime)
            TBuffer::Destruct(pBuffer);
        else
        {
            m_lsGCBuffer.PushBack(pBuffer);
            break;
        }
    }
}

CBufferPool::CBufferPool(DWORD dwPoolSize,
            DWORD dwPoolHold,
            DWORD dwLockTime,
            DWORD dwMaxCacheSize)
: m_dwBufferPoolSize(dwPoolSize)
, m_dwBufferPoolHold(dwPoolHold)
, m_dwBufferLockTime(dwLockTime)
, m_dwMaxCacheSize(dwMaxCacheSize)
{

}

CBufferPool::~CBufferPool()	{Clear();}
