#ifndef CNODEPOOLT_H
#define CNODEPOOLT_H
#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"
#include "TItem.h"
#include "TSimpleList.h"


template<class T> class CNodePoolT
{
public:
    void PutFreeItem(T* pItem)
    {
        ASSERT(pItem != nullptr);

        if(!m_lsFreeItem.TryPut(pItem))
            T::Destruct(pItem);
    }

    void PutFreeItem(TSimpleList<T>& lsItem)
    {
        if(lsItem.IsEmpty())
            return;

        T* pItem;
        while((pItem = lsItem.PopFront()) != nullptr)
            PutFreeItem(pItem);
    }

    T* PickFreeItem()
    {
        T* pItem = nullptr;

        if(m_lsFreeItem.TryGet(&pItem))
            pItem->Reset();
        else
            pItem = T::Construct(m_heap, m_dwItemCapacity);

        return pItem;
    }

    void Prepare()
    {
        m_lsFreeItem.Reset(m_dwPoolHold);
    }

    void Clear()
    {
        T* pItem = nullptr;

        while(m_lsFreeItem.TryGet(&pItem))
            T::Destruct(pItem);

        VERIFY(m_lsFreeItem.IsEmpty());
        m_lsFreeItem.Reset();

        m_heap.Reset();
    }

public:
    void SetItemCapacity(DWORD dwItemCapacity)	{m_dwItemCapacity	= dwItemCapacity;}
    void SetPoolSize	(DWORD dwPoolSize)		{m_dwPoolSize		= dwPoolSize;}
    void SetPoolHold	(DWORD dwPoolHold)		{m_dwPoolHold		= dwPoolHold;}
    DWORD GetItemCapacity	()					{return m_dwItemCapacity;}
    DWORD GetPoolSize		()					{return m_dwPoolSize;}
    DWORD GetPoolHold		()					{return m_dwPoolHold;}

    CPrivateHeap& GetPrivateHeap()				{return m_heap;}

public:
    CNodePoolT(	DWORD dwPoolSize	 = DEFAULT_POOL_SIZE,
                DWORD dwPoolHold	 = DEFAULT_POOL_HOLD,
                DWORD dwItemCapacity = DEFAULT_ITEM_CAPACITY)
                : m_dwPoolSize(dwPoolSize)
                , m_dwPoolHold(dwPoolHold)
                , m_dwItemCapacity(dwItemCapacity)
    {
    }

    ~CNodePoolT()	{Clear();}

    DECLARE_NO_COPY_CLASS(CNodePoolT)

public:
    static const DWORD DEFAULT_ITEM_CAPACITY;
    static const DWORD DEFAULT_POOL_SIZE;
    static const DWORD DEFAULT_POOL_HOLD;

private:
    CPrivateHeap	m_heap;

    DWORD			m_dwItemCapacity;
    DWORD			m_dwPoolSize;
    DWORD			m_dwPoolHold;

    CRingPool<T>	m_lsFreeItem;
};

template<class T> const DWORD CNodePoolT<T>::DEFAULT_ITEM_CAPACITY	= TItem::DEFAULT_ITEM_CAPACITY;
template<class T> const DWORD CNodePoolT<T>::DEFAULT_POOL_SIZE		= 300;
template<class T> const DWORD CNodePoolT<T>::DEFAULT_POOL_HOLD		= 1200;

using CItemPool = CNodePoolT<TItem>;

#endif // CNODEPOOLT_H
