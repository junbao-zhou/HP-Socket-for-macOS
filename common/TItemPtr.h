#ifndef TITEMPTR_H
#define TITEMPTR_H

#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"
#include "TItem.h"
#include "TItemList.h"

struct TItemPtr
{
public:
    TItem* Reset(TItem* pItem = nullptr);

    TItem* Attach(TItem* pItem);

    TItem* Detach();

    TItem* New();

    bool IsValid			()				{return m_pItem != nullptr;}
    TItem* operator ->		()				{return m_pItem;}
    TItem* operator =		(TItem* pItem)	{return Reset(pItem);}
    operator TItem*			()				{return m_pItem;}
    TItem*& PtrRef			()				{return m_pItem;}
    TItem* Ptr				()				{return m_pItem;}
    const TItem* Ptr		()	const		{return m_pItem;}
    operator const TItem*	()	const		{return m_pItem;}

public:
    TItemPtr(CItemPool& pool, TItem* pItem = nullptr);
    TItemPtr(TItemList& ls, TItem* pItem = nullptr);

    ~TItemPtr();

    DECLARE_NO_COPY_CLASS(TItemPtr)

private:
    CItemPool&	itPool;
    TItem*		m_pItem;
};
#endif // TITEMPTR_H
