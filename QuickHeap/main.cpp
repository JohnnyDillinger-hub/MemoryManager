#include <iostream>

#include "QuickHeap.h"

static int average = 0;

__declspec(selectany) QuickHeap g_QuickHeap;

__forceinline void* operator new(size_t n)
{
	//printf("QuickHeap::new size = %d\n", (int)n); 
	return g_QuickHeap.Alloc(n);
}
__forceinline void operator delete(void* p)
{
	//printf("QuickHeap::delete\n"); 
	return g_QuickHeap.Free(p);
}
__forceinline void* operator new[](size_t n)
{
	//printf("QuickHeap::new[] size = %d\n", (int)n); 
	return g_QuickHeap.Alloc(n);
}
__forceinline void operator delete[](void* p)
{
	//printf("QuickHeap::delete[]\n"); 
	return g_QuickHeap.Free(p);
}

class TestObject1
{
public:
	char* ar;

	TestObject1()
	{
		ar = new char[20];
	}

	~TestObject1()
	{
		delete[] ar;
	}
};

class TestObject
{
public:
	char* ar;
	TestObject1* obj1;

	TestObject(int n)
	{
		obj1 = new TestObject1;
		ar = new char[n];
		if (average == 0) {
			average = n;
		}
		else {
			average = (average + n) / 2;
		}
	}

	~TestObject()
	{
		delete obj1;
		delete[] ar;
	}
};

int oldrand = 0;
int Rand()
{
	return ((oldrand = oldrand * 214013L + 2531011L) >> 16) & 0x7fff;
}


int main()
{

	TestObject* temp = new TestObject(20);

}