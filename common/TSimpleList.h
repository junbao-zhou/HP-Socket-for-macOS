#ifndef TSIMPLELIST_H
#define TSIMPLELIST_H

#include "GlobalDef.h"
#include "Singleton.h"
#include "STLHelper.h"
#include "RingBuffer.h"
#include "PrivateHeap.h"
#include "CriSec.h"

template<class T> struct TSimpleList
{
public:
    T* PushFront(T* pItem)
    {
        if(pFront != nullptr)
        {
            pFront->last = pItem;
            pItem->next	 = pFront;
        }
        else
        {
            pItem->last = nullptr;
            pItem->next = nullptr;
            pBack		= pItem;
        }

        pFront = pItem;
        ++size;

        return pItem;
    }

    T* PushBack(T* pItem)
    {
        if(pBack != nullptr)
        {
            pBack->next	= pItem;
            pItem->last	= pBack;
        }
        else
        {
            pItem->last = nullptr;
            pItem->next = nullptr;
            pFront		= pItem;
        }

        pBack = pItem;
        ++size;

        return pItem;
    }

    T* PopFront()
    {
        T* pItem = pFront;

        if(pFront != pBack)
        {
            pFront = pFront->next;
            pFront->last = nullptr;
        }
        else if(pFront != nullptr)
        {
            pFront	= nullptr;
            pBack	= nullptr;
        }

        if(pItem != nullptr)
        {
            pItem->next = nullptr;
            pItem->last = nullptr;

            --size;
        }

        return pItem;
    }

    T* PopBack()
    {
        T* pItem = pBack;

        if(pFront != pBack)
        {
            pBack = pBack->last;
            pBack->next	= nullptr;
        }
        else if(pBack != nullptr)
        {
            pFront	= nullptr;
            pBack	= nullptr;
        }

        if(pItem != nullptr)
        {
            pItem->next = nullptr;
            pItem->last = nullptr;

            --size;
        }

        return pItem;
    }

    TSimpleList<T>& Shift(TSimpleList<T>& other)
    {
        if(&other != this && other.size > 0)
        {
            if(size > 0)
            {
                pBack->next = other.pFront;
                other.pFront->last = pBack;
            }
            else
            {
                pFront = other.pFront;
            }

            pBack	 = other.pBack;
            size	+= other.size;

            other.Reset();
        }

        return *this;
    }

    void Clear()
    {
        if(size > 0)
        {
            T* pItem;
            while((pItem = PopFront()) != nullptr)
                T::Destruct(pItem);
        }
    }

    T*		Front	()	const	{return pFront;}
    T*		Back	()	const	{return pBack;}
    int		Size	()	const	{return size;}
    bool	IsEmpty	()	const	{return size == 0;}

public:
    TSimpleList()	{Reset();}
    ~TSimpleList()	{Clear();}

    DECLARE_NO_COPY_CLASS(TSimpleList<T>)

private:
    void Reset()
    {
        pFront	= nullptr;
        pBack	= nullptr;
        size	= 0;
    }

private:
    int	size;
    T*	pFront;
    T*	pBack;
};


#endif // TSIMPLELIST_H
