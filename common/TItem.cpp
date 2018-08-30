#include "TItem.h"

const DWORD TItem::DEFAULT_ITEM_CAPACITY			= DEFAULT_BUFFER_SIZE;

int	TItem::Cat(const BYTE* pData, int length){
    ASSERT(pData != nullptr && length > 0);

    //获得较小者
    int cat = MIN(Remain(), length);

    if(cat > 0)
    {
        memcpy(end, pData, cat);
        end += cat;
    }

    //返回实际追加信息长度
    return cat;
}

int	TItem::Cat(const TItem& other){
    ASSERT(this != &other);
    return Cat(other.Ptr(), other.Size());
}


//copy到外部缓冲中，会使得begin向后移
int TItem::Fetch(BYTE* pData, int length){
    ASSERT(pData != nullptr && length > 0);

    //获取长度，较小者
    int fetch = MIN(Size(), length);
    memcpy(pData, begin, fetch);
    begin	 += fetch;

    //返回实际拿到的长度
    return fetch;
}

////copy到外部缓冲中，不会移动begin
int	TItem::Peek(BYTE* pData, int length){
    ASSERT(pData != nullptr && length > 0);

    int peek = MIN(Size(), length);
    memcpy(pData, begin, peek);

    //返回实际copy长度
    return peek;
}

/*
    将end向后移动一定长度
*/
int	TItem::Increase(int length){
    ASSERT(length > 0);

    int increase = MIN(Remain(), length);
    end			+= increase;

    //返回实际移动长度
    return increase;
}

/*
  将begin向后移动一定距离
 */
int	TItem::Reduce(int length){
    ASSERT(length > 0);

    int reduce = MIN(Size(), length);
    begin	  += reduce;

    //实际移动距离
    return reduce;
}

void TItem::Reset(int first, int last){
    ASSERT(first >= -1 && first <= capacity);
    ASSERT(last >= -1 && last <= capacity);

    if(first >= 0)	begin	= head + MIN(first, capacity);
    if(last >= 0)	end		= head + MIN(last, capacity);
}

TItem* TItem::Construct(CPrivateHeap& heap,
                            int		capacity,
                            BYTE*	pData,
                            int		length){
        ASSERT(capacity > 0);

        int item_size	= sizeof(TItem);
        TItem* pItem	= (TItem*)heap.Alloc(item_size + capacity);
        BYTE* pHead		= (BYTE*)pItem + item_size;

        return ::ConstructObject(pItem, heap, pHead, capacity, pData, length);
    }


void TItem::Destruct(TItem* pItem){
        ASSERT(pItem != nullptr);

        CPrivateHeap& heap = pItem->heap;
        ::DestructObject(pItem);
        heap.Free(pItem);
    }
