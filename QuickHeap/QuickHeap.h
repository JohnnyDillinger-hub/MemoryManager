#ifndef QUICKHEAP_H
#define QUICKHEAP_H

#include <atlcore.h>

#include"QuickHeapPool.h"

const bool QUICK_HEAP_REALLOC_COPY_DATA = FALSE;
const bool QUICK_HEAP_REALLOC_FILL_ZERO = FALSE;
const int ciQHInitPoolSizeDefault = 0x1000;	// Количество элементов в pool'e
const int ciQHInitPoolArraySizeDefault = 1024; //!должен быть меньше, чем ciQHInitPoolSizeDefault!

class QuickHeap
{
public:
    // Память под QuickHeap должна выделяться в хипе ОС.
    inline void* __cdecl operator new(size_t n)
    {
        return QuickHeapPoolInternalAlloc(n);
    }
    inline void __cdecl operator delete(void* p)
    {
        QuickHeapPoolInternalFree(p);
    }

    // Коструктор.
    QuickHeap()
    {
        // Инициализируем массив пулов значением NULL.
        const int iSisze = sizeof(m_arypQHPool);
        ZeroMemory(m_arypQHPool, iSisze);
    }

    // Деструктор. Освобождает все пулы, занятые в процессе работы QuickHeap.
    ~QuickHeap()
    {
        for (int i = 0; i < sizeof(m_arypQHPool) / sizeof(m_arypQHPool[0]); i++)
            delete (m_arypQHPool[i]);
    }

    // Занимает блок памяти размером cb.
    __forceinline void* Alloc(size_t cb)
    {
        const int iMaxHeapSize = (sizeof(m_arypQHPool) / sizeof(m_arypQHPool[0]));
        // Если размер блока в байтах превышает количество элементов в массиве 
        // пулов, память для него нужно выделять в хипе ОС.
        if (cb >= iMaxHeapSize)
        {
            // Выделяем блок размером cb плюс размер заголовка блока.
            QHBlock* pBlock = (QHBlock*)QuickHeapPoolInternalAlloc(
                sizeof(QuickHeapPool) + cb);

            // Инициализируем указатель на хип значением -1 (0xffffffff для Win32).
            // Впоследствии по этому значению можно будет определить, что память 
            // занята в хипе ОС, а не в пуле.
            pBlock->m_pQuickHeapPool = (QuickHeapPool*)-1;
            // Продвигаем указатель, чтобы он указывал на тело блока.
            ++pBlock;
            return pBlock;

        }
        else if (cb <= 0)
            return NULL; // Если запрашиваю 0 байт, возвращаем NULL.
        else
        {
            // Если блок достаточно мал, выделяем для него память в пуле 
            // подходящего размера.

            // Получаем указатель на пул с блоком соответствующего размера.
            QuickHeapPool* pCurrPool = m_arypQHPool[cb];

            // Если пул еще не создан, указатель будет содержать NULL.
            if (!pCurrPool) // В этом случае...
              // ...создаем новый пул и помещаем указатель на него в массив.
                pCurrPool = m_arypQHPool[cb] =
                new QuickHeapPool(cb, ciQHInitPoolSizeDefault / (int)cb);

            // Выделяем блок из пула.
            return pCurrPool->Alloc();
        }
    }
    // Освобождает блок памяти ранее занятый функцией Alloc.
    __forceinline void Free(void* p)
    {
        // Получаем указатель на заголовок блока.
        QHBlock* pBlock = (QHBlock*)p;
        pBlock--; // Заголовок находится по отрицательному смещению.

        // Получаем хип, в котором был занят блок.
        QuickHeapPool* pQuickHeapPool = (QuickHeapPool*)pBlock->m_pQuickHeapPool;

        // Если блок был занят в системном пуле, даем ему слово...
        if ((QuickHeapPool*)-1 == pQuickHeapPool)
            QuickHeapPoolInternalFree(pBlock);
        // Такая проверка не нужна, так как такая ситуация исключена.
        //else if(NULL == pQuickHeapPool)
        //  return;
        else
            // Освобождаем блок (при этом он помещается в список свободных блоков).
            pQuickHeapPool->FreeBlock(pBlock);
    }

    // Данные QuickHeap.

    // Массив пулов. Ячейка этого массива содержит NULL или указатель на пул
    // с размером блока, равным индексу этой ячейки. 
    QuickHeapPool* m_arypQHPool[1024];
};


#endif
