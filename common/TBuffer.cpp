#include "TBuffer.h"
#include "CBufferPool.h"

TBuffer* TBuffer::Construct(CBufferPool& pool, ULONG_PTR dwID){
    ASSERT(dwID != 0);

    CPrivateHeap& heap	= pool.GetPrivateHeap();
    TBuffer* pBuffer	= (TBuffer*)heap.Alloc(sizeof(TBuffer));

    return ::ConstructObject(pBuffer, heap, pool.GetItemPool(), dwID);
}

void TBuffer::Destruct(TBuffer* pBuffer){
    ASSERT(pBuffer != nullptr);

    CPrivateHeap& heap = pBuffer->heap;
    ::DestructObject(pBuffer);
    heap.Free(pBuffer);
}

int TBuffer::Cat(const BYTE* pData, int len){
    items.Cat(pData, len);
    return IncreaseLength(len);
}

int TBuffer::Cat(const TItem* pItem){
    items.Cat(pItem);
    return IncreaseLength(pItem->Size());
}

int TBuffer::Cat(const TItemList& other){
    ASSERT(&items != &other);

    for(TItem* pItem = other.Front(); pItem != nullptr; pItem = pItem->next)
        Cat(pItem);

    return length;
}

int TBuffer::Fetch(BYTE* pData, int length){
    int fetch = items.Fetch(pData, length);
    DecreaseLength(fetch);

    return fetch;
}

int TBuffer::Peek(BYTE* pData, int length){
    return items.Peek(pData, length);
}

int TBuffer::Reduce(int len){
    int reduce = items.Reduce(len);
    DecreaseLength(reduce);

    return reduce;
}

void TBuffer::Reset(){
    id		 = 0;
    length	 = 0;
    freeTime = ::TimeGetTime();
}

TBuffer::TBuffer(CPrivateHeap& hp, CItemPool& itPool, ULONG_PTR dwID)
: heap(hp), items(itPool), id(dwID), length(0)
{
}

TBuffer::~TBuffer()	{}
