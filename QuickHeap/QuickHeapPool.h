#ifndef QUICKHEAPPOOL_H
#define QUICKHEAPPOOL_H

#define QUICK_HEAP_SYNCHRONIZATION

#ifdef QUICK_HEAP_SYNCHRONIZATION
#include <atlcore.h>

#endif // QUICK_HEAP_SYNCHRONIZATION

// ��� ��������� ����������� � �������� �������� � ������������� ������
// ���� �������� ASC_MEM_STATISTIC_USE_QUICK_HEAP � ����������� �������
// OnQuickHeapInternalAlloc � OnQuickHeapPoolInternalFree
// ����� #include "QuickHeap.h"
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
	QHBlock* m_pNext;           // ��������� ��������� ����
	void* m_pQuickHeapPool;     // ����� ���� (����� �����)
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
  /* ������ ��� ����� ������ ������ ���������� � ���� ��, ��� ��� QuickHeap
   * �� ����� �������� ������ ��� ��� ����. ��� ��� ��� �������������
   * QuickHeap ������ ���������������� ���������� ��������� new � delete, �
   * ���� ������ ����� ������� �� �������������� � ��������� �������� ������
   * � ���� ��. ������� QuickHeapPoolInternalXxx �������� ������ � �������
   * ���� ��������.
   */
	inline void* __cdecl operator new(size_t n)
	{
		return QuickHeapPoolInternalAlloc(n);
	}
	inline void   __cdecl operator delete(void* p)
	{
		QuickHeapPoolInternalFree(p);
	}
	// Alloc �������� ����� ��� ������.
	void* Alloc()
	{
		QHBlock* p = AllocBlock();

	/* ������ ���� ���������� � ���������. ������� ����� ���, ��� ������ ����
	 * �����������, ���������� �������� ��������� �� �������, ������
	 * ��������������� �� ���������� �����. ��� ��� ������ ��������� �����
	 * ������� ��������� QHBlock, ����� ������ ��������� ��������� ��
	 * ���� (p) �� �������.
	 */
		p++;
		return p;
	}
	void Free(void* p)
	{
		ATLASSERT(0);
		FreeBlock((QHBlock*)(BYTE(p) - sizeof(QHBlock*)));
	}

	// �����������.
	QuickHeapPool(size_t BlockSize,		 // ������ ����� � ����.
				  int iInitBlockCnt = 0) // ��������� ���������� ������.
		: m_CurrExtent(NULL), m_pCurrentBlock(NULL), m_pFreeBlocksList(NULL)
	{
		// ������������ �������� ������ �����. ������ �� ����� ����������� �
		// ���������� ��������.
		m_iIntrBlockSize = sizeof(QHBlock) + BlockSize;

		// ���� ������ ��������� ���������� ������, ...
		if (iInitBlockCnt > 0)
			AddExtent(iInitBlockCnt); // ...����� �� ��������� ������ �������
	}

	// ����������. ����������� ������� ��������.
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

	// �������� ���� ������.
	__forceinline QHBlock* AllocBlock()
	{
		// ���� ������ ������������� ������ �� ����...s
		if (m_pFreeBlocksList)
		{
			// �������� �� ���� ������ ������� � ������ ��� �����������.
			// ��� ��������� ����������� �������� �������������
			// �����/������������ ������.
			register QHBlock* pBlock = m_pFreeBlocksList;
			m_pFreeBlocksList = m_pFreeBlocksList->m_pNext;
			pBlock->m_pQuickHeapPool = this;
			return pBlock;
		}
		else
			return GetBlock(); // ����� ����� ����� ���� �� �������� ��������.
	}

	// ����������� ���� ����� ������� ����� AllocBlock ����.
	__forceinline void FreeBlock(QHBlock* pBlock)
	{
		// ������������ � QuickHeap �������� � ���������� ����� � ������
		// ������������� ������.
		pBlock->m_pNext = m_pFreeBlocksList;
		m_pFreeBlocksList = pBlock;
	}

	__forceinline size_t GetBlockSize()
	{	// �������� ������ ����� m_iIntrBlockSize - sizeof(QHBlock)
		return m_iIntrBlockSize;
	}

protected:

	// AddExtent �������� ����� ������� �������� iBlockCnt ������.
	// ������ ��� ������� ���������� � ���� ��. � ��������, ��� ����� � �������
	// �������� ����� ������ ����� ���� �� �������� ����� � �� 
	// (��������� VirtualAlloc/ VirtualFree).
	void AddExtent(size_t iBlockCnt)
	{
		ATLASSERT(!m_CurrExtent
			|| m_pCurrentBlock == m_CurrExtent->m_aryBlocks);

		// �������� ������ ��� ������� (��������� � �������� � ������� �����).
		QHExtent* pExtent = (QHExtent*)QuickHeapPoolInternalAlloc(
			sizeof(QHExtent) + (iBlockCnt/* + 1*/)*m_iIntrBlockSize);

		// �������� ��������� �� ���������� ������� � ���������� m_pNextExtent
		// ������ ��������.
		pExtent->m_pNextExtent = m_CurrExtent;
		ATLASSERT(iBlockCnt > 0);

		// ���������� � m_pCurrent ��������� �� ����� ��������� ����.
		// ���� ��������� �������������� ��� ����� ������� ����� ���� ����������
		// ������, ���������� �� �� ������. ���������� � BYTE* ��������� ��-�� 
		// ����, ��� �������� �������������� � ������. ���� ����� �� �������, �� 
		// �������� C++ �������� ����� ������������� �� ������ ��������� QHBlock.
		m_pCurrentBlock = (QHBlock*)((BYTE*)(pExtent->m_aryBlocks)
			+ (iBlockCnt - 1) * m_iIntrBlockSize);

		// ������ ����� ������� �������. ��� ����� ���������� �������� 
		// ����� ������ �� ����.
		m_CurrExtent = pExtent;

		// ���������� ������ �������� �������� ��� ����, ����� � ��������� ��� 
		// ������� ���� ������.
		m_iCurrExtentSize = iBlockCnt;
	}

	// �������� ���� ����� ���� (�� ��������).
	__forceinline QHBlock* GetBlock()
	{
		// ���������, �� �������� �� ������� �������...
		if (m_pCurrentBlock == m_CurrExtent->m_aryBlocks)
			// ���� ��������, ��������� ����� �������, ���������� ��������� 
			// � ������� ����� ������, ��� � ����������.
			AddExtent(m_iCurrExtentSize * 2);
		QHBlock* pRet = m_pCurrentBlock;

		// �������������� ��������� ������ �����.
		// ��������� �� ��� ����� ����� ��� ������������ �����, ��� ��� 
		// � ���������� ������������ ���������� ������ ������������� ����.
		pRet->m_pQuickHeapPool = this;

		// ��������� ��������� �� ������� ����, ������� ��� ��� ����� ����� �
		// m_aryBlocks. ��� ��� ������ ����� ������������ � ������, ���������� 
		// �������� m_pCurrent � ��������� �� ���� (BYTE*). ����� ��������
		// lvalue ����������� ���� ������ �&�.
		(BYTE*&)m_pCurrentBlock -= m_iIntrBlockSize;
		return pRet;
	}
	// ������ ����.

  // ���������� ������ � ������� ��������. ����� ��� ������� ���������� 
  // ������ � ��������� ��������.
  size_t m_iCurrExtentSize;
  // ���������� ������ �����. �������� � ���� ������ �����, ����������
  // � ������������, � ������ ��������� �����.
  size_t m_iIntrBlockSize;
  // ������� ������� ����. ������ ����� ���������� ��������� �������� 
  // � ���� ���������� ������ � ���� QHExtent::m_pNextExtent
  QHExtent * m_CurrExtent;
  // ������ ������������� ������. ����� ����������� ����������� �����
  // ������� ����, ���� ���� �������� ���������� � ������ ���������� ������
  // ��������� ������. ��� ��������� ������� ���� �� ���������� �� ��������
  // ��������, � ������������ �� ����� ������, ������� ������ ��� ����
  // ���������� ����, ��������� �� ������� ��������� � QHBlock::m_pNext. 
  // ���� ��������� ������ ���, � m_pFreeBlocksList ���������� NULL.
  QHBlock * m_pFreeBlocksList;

  QHBlock* m_pCurrentBlock;
};

#endif