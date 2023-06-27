#ifndef QUICKHEAPPOOL_H
#define QUICKHEAPPOOL_H

#define QUICK_HEAP_SYNCHRONIZATION

#ifdef QUICK_HEAP_SYNCHRONIZATION
#include <atlcore.h>

#endif // QUICK_HEAP_SYNCHRONIZATION

// Для получения уведомлений о реальных занятиях и освобождениях памяти
// надо объявить ASC_MEM_STATISTIC_USE_QUICK_HEAP и реализовать функции
// OnQuickHeapInternalAlloc и OnQuickHeapPoolInternalFree
// перед #include "QuickHeap.h"
inline void* QuickHeapPoolInternalAlloc(size_t size)
{
    void* p = HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, size);
    //FillMemory(p, size, 0xfe);
#ifdef ASC_MEM_STATISTIC_USE_QUICK_HEAP
    OnQuickHeapInternalAlloc(p, size);
#endif	//ASC_MEM_STATISTIC_USE_QUICK_HEAP
    return p;
}
inline void* QuickHeapPoolInternalReAlloc(void* p, size_t size)
{
    void* pRet = HeapReAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, p, size);
#ifdef ASC_MEM_STATISTIC_USE_QUICK_HEAP
    ATLASSERT(0);
#endif	//ASC_MEM_STATISTIC_USE_QUICK_HEAP
    return pRet;
}
inline void QuickHeapPoolInternalFree(void* pBlock)
{
#ifdef ASC_MEM_STATISTIC_USE_QUICK_HEAP
    OnQuickHeapPoolInternalFree(pBlock);
#endif	//ASC_MEM_STATISTIC_USE_QUICK_HEAP
    HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, pBlock);
}

struct QHBlock
{
	QHBlock* m_pNext;           // Следующий свободный блок
	void* m_pQuickHeapPool;     // Адрес пула (когда занят)
protected:
	inline void* __cdecl operator new(size_t n) { n; }
	inline void   __cdecl operator delete(void* p) { p; }
};

struct QHExtent
{
	QHExtent* m_pNextExtent;
	QHBlock* m_pCurrent;

	QHBlock m_aryBlocks[0];
protected:
	inline void* __cdecl operator new(size_t n) { n; }
	inline void   __cdecl operator delete(void* p) { p; }
};

class QuickHeapPool
{
public:
  /* Память для этого класса должна выделяться в хипе ОС, так как QuickHeap
   * не может выделять память сам для себя. Так как для использования
   * QuickHeap обычно переопределяются глобальные операторы new и delete, в
   * этом классе нужно вручную их переопределить и заставить выделять память
   * в хипе ОС. Функции QuickHeapPoolInternalXxx выделяют память в главном
   * хипе процесса.
   */
	inline void* __cdecl operator new(size_t n)
	{
		return QuickHeapPoolInternalAlloc(n);
	}
	inline void   __cdecl operator delete(void* p)
	{
		QuickHeapPoolInternalFree(p);
	}
	// Alloc занимает новый бок памяти.
	void* Alloc()
	{
		QHBlock* p = AllocBlock();

	/* Каждый блок начинается с заголовка. Поэтому перед тем, как отдать блок
	 * потребителю, необходимо сместить указатель на область, идущую
	 * непосредственно за заголовком блока. Так как размер заголовка равен
	 * размеру структуры QHBlock, можно просто увеличить указатель на
	 * блок (p) на единицу.
	 */
		p++;
		return p;
	}
	void Free(void* p)
	{
		ATLASSERT(0);
		FreeBlock((QHBlock*)(BYTE(p) - sizeof(QHBlock*)));
	}

	// Конструктор.
	QuickHeapPool(size_t BlockSize,		 // Размер блока в пуле.
				  int iInitBlockCnt = 0) // Начальное количество блоков.
		: m_CurrExtent(NULL), m_pCurrentBlock(NULL), m_pFreeBlocksList(NULL)
	{
		// Рассчитываем реальный размер блока. Именно он будет участвовать в
		// дальнейших расчетах.
		m_iIntrBlockSize = sizeof(QHBlock) + BlockSize;

		// Если задано начальное количество блоков, ...
		if (iInitBlockCnt > 0)
			AddExtent(iInitBlockCnt); // ...сразу же добавляем первый экстент
	}

	// Деструктор. Освобождает занятые экстенты.
	~QuickHeapPool(void)
	{
		QHExtent* pExtent = m_CurrExtent;
		while (pExtent)
		{
			QHExtent* pNext = pExtent->m_pNextExtent;
			QuickHeapPoolInternalFree(pExtent);
			pExtent = pNext;
		}
	}

	// Занимает блок памяти.
	__forceinline QHBlock* AllocBlock()
	{
		// Если список освобожденных блоков не пуст...s
		if (m_pFreeBlocksList)
		{
			// Вынимаем из него первый элемент и отдаем его потребителю.
			// Это позволяет существенно ускорить множественные
			// займы/освобождения памяти.
			register QHBlock* pBlock = m_pFreeBlocksList;
			m_pFreeBlocksList = m_pFreeBlocksList->m_pNext;
			pBlock->m_pQuickHeapPool = this;
			return pBlock;
		}
		else
			return GetBlock(); // Иначе берем новый блок из текущего экстента.
	}

	// Освобождает блок ранее занятый через AllocBlock блок.
	__forceinline void FreeBlock(QHBlock* pBlock)
	{
		// Освобождение в QuickHeap сводится к добавлению блока в список
		// освобожденных блоков.
		pBlock->m_pNext = m_pFreeBlocksList;
		m_pFreeBlocksList = pBlock;
	}

	__forceinline size_t GetBlockSize()
	{	// Реальный размер блока m_iIntrBlockSize - sizeof(QHBlock)
		return m_iIntrBlockSize;
	}

protected:

	// AddExtent занимает новый экстент размером iBlockCnt блоков.
	// Память под экстент выделяется в хипе ОС. В принципе, для пулов с большим
	// размером блока память можно было бы выделять прямо в ОС 
	// (функциями VirtualAlloc/ VirtualFree).
	void AddExtent(size_t iBlockCnt)
	{
		ATLASSERT(!m_CurrExtent
			|| m_pCurrentBlock == m_CurrExtent->m_aryBlocks);

		// Занимаем память под экстент (заголовок и входящие в экстент блоки).
		QHExtent* pExtent = (QHExtent*)QuickHeapPoolInternalAlloc(
			sizeof(QHExtent) + (iBlockCnt/* + 1*/)*m_iIntrBlockSize);

		// Помещаем указатель на предыдущий экстент в переменную m_pNextExtent
		// нового экстента.
		pExtent->m_pNextExtent = m_CurrExtent;
		ATLASSERT(iBlockCnt > 0);

		// Записываем в m_pCurrent указатель на самый последний блок.
		// Этот указатель рассчитывается как адрес первого блока плюс количество
		// блоков, умноженное на их размер. Приведение к BYTE* требуется из-за 
		// того, что смещение рассчитывается в байтах. Если этого не сделать, по 
		// правилам C++ смещение будет увеличиваться на размер структуры QHBlock.
		m_pCurrentBlock = (QHBlock*)((BYTE*)(pExtent->m_aryBlocks)
			+ (iBlockCnt - 1) * m_iIntrBlockSize);

		// Делаем новый экстент текущим. Тем самым заставляем выделять 
		// блоки именно из него.
		m_CurrExtent = pExtent;

		// Запоминаем размер текущего экстента для того, чтобы в следующий раз 
		// удвоить этот размер.
		m_iCurrExtentSize = iBlockCnt;
	}

	// Выделяет блок новый блок (из экстента).
	__forceinline QHBlock* GetBlock()
	{
		// Проверяем, не исчерпан ли текущий экстент...
		if (m_pCurrentBlock == m_CurrExtent->m_aryBlocks)
			// Если исчерпан, добавляем новый экстент, количество элементов 
			// в котором вдвое больше, чем в предыдущем.
			AddExtent(m_iCurrExtentSize * 2);
		QHBlock* pRet = m_pCurrentBlock;

		// Инициализируем заголовок нового блока.
		// Указатель на пул будет нужен при освобождении блока, так как 
		// в процедурах освобождения передается только освобождаемый блок.
		pRet->m_pQuickHeapPool = this;

		// Уменьшаем указатель на текущий блок, сдвигая его тем самым назад к
		// m_aryBlocks. Так как размер блока определяется в байтах, необходимо 
		// привести m_pCurrent к указателю на байт (BYTE*). Чтобы получить
		// lvalue добавляется знак ссылки «&».
		(BYTE*&)m_pCurrentBlock -= m_iIntrBlockSize;
		return pRet;
	}
	// Данные пула.

  // Количество блоков в текущем экстенте. Нужен для расчета количества 
  // блоков в следующем экстенте.
  size_t m_iCurrExtentSize;
  // Внутренний размер блока. Включает в себя размер блока, получаемый
  // в конструкторе, и размер заголовка блока.
  size_t m_iIntrBlockSize;
  // Текущий экстент пула. Список ранее выделенных экстентов хранится 
  // в виде связанного списка в поле QHExtent::m_pNextExtent
  QHExtent * m_CurrExtent;
  // Список освобожденных блоков. Когда программист освобождает ранее
  // занятый блок, этот блок попросту помещается в начало связанного списка
  // свободных блоков. При следующем запросе блок не выделяется из текущего
  // экстента, а возвращается из этого списка, головой списка при этом
  // становится блок, указатель на который находится в QHBlock::m_pNext. 
  // Если свободных блоков нет, в m_pFreeBlocksList помещается NULL.
  QHBlock * m_pFreeBlocksList;

  QHBlock* m_pCurrentBlock;
};

#endif