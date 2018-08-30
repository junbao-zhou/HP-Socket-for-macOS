#include "TItemList.h"

/*
    在尾部追加数据，不保证全部追加
 */
int TItemList::PushTail(const BYTE* pData, int length){
    ASSERT(length <= (int)itPool.GetItemCapacity());

    if(length > (int)itPool.GetItemCapacity())
        return 0;

    //从缓冲池中得到
    //此处一定得到一个返回对象 !=null
    //若池中不存在则会使用C函数的Malloc
    TItem* pItem = PushBack(itPool.PickFreeItem());
    //实际被添加的长度
    return pItem->Cat(pData, length);
}

/*
  追加数据，保证数据全部添加
 */
int TItemList::Cat(const BYTE* pData, int length){
    int remain = length;

    while(remain > 0)
    {
        TItem* pItem = Back();

        if(pItem == nullptr || pItem->IsFull())
            pItem = PushBack(itPool.PickFreeItem());

        int cat  = pItem->Cat(pData, remain);

        pData	+= cat;
        remain	-= cat;
    }

    return length;
}

int TItemList::Cat(const TItem* pItem){
    return Cat(pItem->Ptr(), pItem->Size());
}

int TItemList::Cat(const TItemList& other){
    ASSERT(this != &other);

    int length = 0;

    for(TItem* pItem = other.Front(); pItem != nullptr; pItem = pItem->next)
        length += Cat(pItem);

    return length;
}

int TItemList::Fetch(BYTE* pData, int length){
    int remain = length;

    //Size保证Front不为nullptr
    while(remain > 0 && Size() > 0)
    {
        TItem* pItem = Front();
        int fetch	 = pItem->Fetch(pData, remain);

        pData	+= fetch;
        remain	-= fetch;

        //为空后将元素返还给缓冲池
        if(pItem->IsEmpty())
            itPool.PutFreeItem(PopFront());
    }

    //返回实际拉取长度
    return length - remain;
}

/*
    不会使pFront改变，只copy数据
 */
int TItemList::Peek(BYTE* pData, int length){
    int remain	 = length;
    TItem* pItem = Front();

    while(remain > 0 && pItem != nullptr)
    {
        int peek = pItem->Peek(pData, remain);

        pData	+= peek;
        remain	-= peek;
        pItem	 = pItem->next;
    }

    //返回实际copy长度
    return length - remain;
}

/*
  移动一定长度，改变pFront
 */
int TItemList::Reduce(int length){
    int remain = length;

    while(remain > 0 && Size() > 0)
    {
        TItem* pItem = Front();
        remain		-= pItem->Reduce(remain);

        if(pItem->IsEmpty())
            itPool.PutFreeItem(PopFront());
    }

    return length - remain;
}

/*
  释放所有元素
 */
void TItemList::Release(){
    itPool.PutFreeItem(*this);
}

TItemList::TItemList(CItemPool& pool) : itPool(pool)
{
}
