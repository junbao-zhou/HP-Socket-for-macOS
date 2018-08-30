#ifndef TITEMLISTEXT_H
#define TITEMLISTEXT_H
#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"
#include "TItemList.h"

template<class length_t = int, typename = enable_if_t<is_integral<typename decay<length_t>::type>::value>>
struct TItemListExT : public TItemList
{
    using __super = TItemList;

public:
    TItem* PushFront(TItem* pItem)
    {
        length += pItem->Size();
        return __super::PushFront(pItem);
    }

    TItem* PushBack(TItem* pItem)
    {
        length += pItem->Size();
        return __super::PushBack(pItem);
    }

    TItem* PopFront()
    {
        TItem* pItem = __super::PopFront();

        if(pItem != nullptr)
            length -= pItem->Size();

        return pItem;
    }

    TItem* PopBack()
    {
        TItem* pItem = __super::PopBack();

        if(pItem != nullptr)
            length -= pItem->Size();

        return pItem;
    }

    TItemListExT& Shift(TItemListExT& other)
    {
        if(&other != this && other.length > 0)
        {
            length += other.length;
            other.length = 0;

            __super::Shift(other);
        }

        return *this;
    }

    void Clear()
    {
        __super::Clear();
        length = 0;
    }

    void Release()
    {
        __super::Release();
        length = 0;
    }

public:
    int PushTail(const BYTE* pData, int length)
    {
        int cat = __super::PushTail(pData, length);
        this->length += cat;

        return cat;
    }

    int Cat(const BYTE* pData, int length)
    {
        int cat = __super::Cat(pData, length);
        this->length += cat;

        return cat;
    }

    int Cat(const TItem* pItem)
    {
        int cat = __super::Cat(pItem->Ptr(), pItem->Size());
        this->length += cat;

        return cat;
    }

    int Cat(const TItemList& other)
    {
        int cat = __super::Cat(other);
        this->length += cat;

        return cat;
    }

    int Fetch(BYTE* pData, int length)
    {
        int fetch	  = __super::Fetch(pData, length);
        this->length -= fetch;

        return fetch;
    }

    int Reduce(int length)
    {
        int reduce	  = __super::Reduce(length);
        this->length -= reduce;

        return reduce;
    }

    typename decay<length_t>::type Length() const {return length;}

public:
    TItemListExT(CItemPool& pool) : TItemList(pool), length(0)
    {
    }

    ~TItemListExT()
    {
        ASSERT(length >= 0);
    }

    DECLARE_NO_COPY_CLASS(TItemListExT)

private:
    length_t length;
};

using TItemListEx	= TItemListExT<>;
using TItemListExV	= TItemListExT<volatile int>;

#endif // TITEMLISTEXT_H
