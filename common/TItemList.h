#ifndef TITEMLIST_H
#define TITEMLIST_H
#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"
#include "TSimpleList.h"
#include "TItem.h"
#include "CNodePoolT.h"

struct TItemList : public TSimpleList<TItem>
{
    friend struct TItemPtr;

public:
    /*
        在尾部追加数据，不保证全部追加
     */
    int PushTail(const BYTE* pData, int length);

    /*
      追加数据，保证数据全部添加
     */
    int Cat(const BYTE* pData, int length);

    int Cat(const TItem* pItem);

    int Cat(const TItemList& other);

    int Fetch(BYTE* pData, int length);

    /*
        不会使pFront改变，只copy数据
     */
    int Peek(BYTE* pData, int length);

    /*
      移动一定长度，改变pFront
     */
    int Reduce(int length);

    /*
      释放所有元素
     */
    void Release();

public:
    TItemList(CItemPool& pool);

private:
    CItemPool& itPool;
};

#endif // TITEMLIST_H
