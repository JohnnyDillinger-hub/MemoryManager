#ifndef QUICKHEAP_H
#define QUICKHEAP_H

#include <atlcore.h>

#include"QuickHeapPool.h"

const bool QUICK_HEAP_REALLOC_COPY_DATA = FALSE;
const bool QUICK_HEAP_REALLOC_FILL_ZERO = FALSE;
const int ciQHInitPoolSizeDefault = 0x1000;	// ���������� ��������� � pool'e
const int ciQHInitPoolArraySizeDefault = 1024; //!������ ���� ������, ��� ciQHInitPoolSizeDefault!

class QuickHeap
{
public:
    // ������ ��� QuickHeap ������ ���������� � ���� ��.
    inline void* __cdecl operator new(size_t n)
    {
        return QuickHeapPoolInternalAlloc(n);
    }
    inline void __cdecl operator delete(void* p)
    {
        QuickHeapPoolInternalFree(p);
    }

    // ����������.
    QuickHeap()
    {
        // �������������� ������ ����� ��������� NULL.
        const int iSisze = sizeof(m_arypQHPool);
        ZeroMemory(m_arypQHPool, iSisze);
    }

    // ����������. ����������� ��� ����, ������� � �������� ������ QuickHeap.
    ~QuickHeap()
    {
        for (int i = 0; i < sizeof(m_arypQHPool) / sizeof(m_arypQHPool[0]); i++)
            delete (m_arypQHPool[i]);
    }

    // �������� ���� ������ �������� cb.
    __forceinline void* Alloc(size_t cb)
    {
        const int iMaxHeapSize = (sizeof(m_arypQHPool) / sizeof(m_arypQHPool[0]));
        // ���� ������ ����� � ������ ��������� ���������� ��������� � ������� 
        // �����, ������ ��� ���� ����� �������� � ���� ��.
        if (cb >= iMaxHeapSize)
        {
            // �������� ���� �������� cb ���� ������ ��������� �����.
            QHBlock* pBlock = (QHBlock*)QuickHeapPoolInternalAlloc(
                sizeof(QuickHeapPool) + cb);

            // �������������� ��������� �� ��� ��������� -1 (0xffffffff ��� Win32).
            // ������������ �� ����� �������� ����� ����� ����������, ��� ������ 
            // ������ � ���� ��, � �� � ����.
            pBlock->m_pQuickHeapPool = (QuickHeapPool*)-1;
            // ���������� ���������, ����� �� �������� �� ���� �����.
            ++pBlock;
            return pBlock;

        }
        else if (cb <= 0)
            return NULL; // ���� ���������� 0 ����, ���������� NULL.
        else
        {
            // ���� ���� ���������� ���, �������� ��� ���� ������ � ���� 
            // ����������� �������.

            // �������� ��������� �� ��� � ������ ���������������� �������.
            QuickHeapPool* pCurrPool = m_arypQHPool[cb];

            // ���� ��� ��� �� ������, ��������� ����� ��������� NULL.
            if (!pCurrPool) // � ���� ������...
              // ...������� ����� ��� � �������� ��������� �� ���� � ������.
                pCurrPool = m_arypQHPool[cb] =
                new QuickHeapPool(cb, ciQHInitPoolSizeDefault / (int)cb);

            // �������� ���� �� ����.
            return pCurrPool->Alloc();
        }
    }
    // ����������� ���� ������ ����� ������� �������� Alloc.
    __forceinline void Free(void* p)
    {
        // �������� ��������� �� ��������� �����.
        QHBlock* pBlock = (QHBlock*)p;
        pBlock--; // ��������� ��������� �� �������������� ��������.

        // �������� ���, � ������� ��� ����� ����.
        QuickHeapPool* pQuickHeapPool = (QuickHeapPool*)pBlock->m_pQuickHeapPool;

        // ���� ���� ��� ����� � ��������� ����, ���� ��� �����...
        if ((QuickHeapPool*)-1 == pQuickHeapPool)
            QuickHeapPoolInternalFree(pBlock);
        // ����� �������� �� �����, ��� ��� ����� �������� ���������.
        //else if(NULL == pQuickHeapPool)
        //  return;
        else
            // ����������� ���� (��� ���� �� ���������� � ������ ��������� ������).
            pQuickHeapPool->FreeBlock(pBlock);
    }

    // ������ QuickHeap.

    // ������ �����. ������ ����� ������� �������� NULL ��� ��������� �� ���
    // � �������� �����, ������ ������� ���� ������. 
    QuickHeapPool* m_arypQHPool[1024];
};


#endif
