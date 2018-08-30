#ifndef TITEM_H
#define TITEM_H

#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"

struct TItem
{
    template<typename T> friend struct	TSimpleList;
    template<typename T> friend class	CNodePoolT;

    friend struct	TItemList;
    friend struct	TBuffer;

public:
    int	Cat(const BYTE* pData, int length);

    int	Cat(const TItem& other);

    //copy到外部缓冲中，会使得begin向后移
    int	Fetch(BYTE* pData, int length);

    ////copy到外部缓冲中，不会移动begin
    int	Peek(BYTE* pData, int length);

    /*
        将end向后移动一定长度
    */
    int	Increase(int length);

    /*
      将begin向后移动一定距离
     */
    int	Reduce(int length);

    void Reset(int first = 0, int last = 0);

    BYTE*		Ptr		()			{return begin;}
    const BYTE*	Ptr		()	const	{return begin;}
    int			Size	()	const	{return (int)(end - begin);}
    int			Remain	()	const	{return capacity - (int)(end - head);}
    int			Capacity()	const	{return capacity;}
    bool		IsEmpty	()	const	{return Size()	 == 0;}
    bool		IsFull	()	const	{return Remain() == 0;}

public:
    operator		BYTE*	()			{return Ptr();}
    operator const	BYTE*	() const	{return Ptr();}

public:
    static TItem* Construct(CPrivateHeap& heap,
                            int		capacity	= DEFAULT_ITEM_CAPACITY,
                            BYTE*	pData		= nullptr,
                            int		length		= 0);


    static void Destruct(TItem* pItem);

private:
    friend TItem* ConstructObject<>(TItem*, CPrivateHeap&, BYTE*&, int&, BYTE*&, int&);
    friend void DestructObject<>(TItem*);

    TItem(CPrivateHeap& hp, BYTE* pHead, int cap = DEFAULT_ITEM_CAPACITY, BYTE* pData = nullptr, int length = 0)
    : heap(hp), head(pHead), begin(pHead), end(pHead), capacity(cap), next(nullptr), last(nullptr)
    {
        if(pData != nullptr && length != 0)
            Cat(pData, length);
    }

    ~TItem() {}

    DECLARE_NO_COPY_CLASS(TItem)

public:
    static const DWORD DEFAULT_ITEM_CAPACITY;

private:
    CPrivateHeap& heap;

private:
    TItem* next;
    TItem* last;

    int		capacity;
    BYTE*	head;
    BYTE*	begin;
    BYTE*	end;
};
#endif // TITEM_H
