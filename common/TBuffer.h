#ifndef TBUFFER_H
#define TBUFFER_H
#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"
#include "TItemList.h"

class CBufferPool;
/*
  基于TItemList的封装，CBufferPool的基础单位
  可见，大部分的封装可以提供全新的概念
 */
struct TBuffer
{
    template<typename T> friend struct	TSimpleList;
    friend class						CBufferPool;

public:
    static TBuffer* Construct(CBufferPool& pool, ULONG_PTR dwID);

    static void Destruct(TBuffer* pBuffer);
public:
    int Cat(const BYTE* pData, int len);

    int Cat(const TItem* pItem);

    int Cat(const TItemList& other);

    int Fetch(BYTE* pData, int length);

    int Peek(BYTE* pData, int length);

    int Reduce(int len);
public:
    CCriSec&	CriSec	()	{return cs;}
    TItemList&	ItemList()	{return items;}

    ULONG_PTR ID		()	const	{return id;}
    int Length			()	const	{return length;}
    bool IsValid		()	const	{return id != 0;}

private:
    int IncreaseLength	(int len)	{return (length += len);}
    int DecreaseLength	(int len)	{return (length -= len);}

    void Reset();

private:
    friend TBuffer* ConstructObject<>(TBuffer*, CPrivateHeap&, CItemPool&, ULONG_PTR&);
    friend void DestructObject<>(TBuffer*);

    TBuffer(CPrivateHeap& hp, CItemPool& itPool, ULONG_PTR dwID = 0);

    ~TBuffer();

    DECLARE_NO_COPY_CLASS(TBuffer)

private:
    CPrivateHeap&	heap;

private:
    ULONG_PTR		id;
    int				length;
    DWORD			freeTime;

private:
    TBuffer*		next;
    TBuffer*		last;

    CCriSec			cs;
    TItemList		items;
};


#endif // TBUFFER_H
