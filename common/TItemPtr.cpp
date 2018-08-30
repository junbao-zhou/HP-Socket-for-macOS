#include "TItemPtr.h"

TItem* TItemPtr::Reset(TItem* pItem)
{
    if(m_pItem != nullptr)
        itPool.PutFreeItem(m_pItem);

    m_pItem = pItem;

    return m_pItem;
}

TItem* TItemPtr::Attach(TItem* pItem)
{
    return Reset(pItem);
}

TItem* TItemPtr::Detach()
{
    TItem* pItem = m_pItem;
    m_pItem		 = nullptr;

    return pItem;
}

TItem* TItemPtr::New()
{
    return Attach(itPool.PickFreeItem());
}

TItemPtr::TItemPtr(CItemPool& pool, TItem* pItem)
: itPool(pool), m_pItem(pItem)
{

}

TItemPtr::TItemPtr(TItemList& ls, TItem* pItem)
: itPool(ls.itPool), m_pItem(pItem)
{

}

TItemPtr::~TItemPtr()
{
    Reset();
}
