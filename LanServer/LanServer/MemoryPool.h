#pragma once
#include <Windows.h>


template<class Type>
class CFreeList
{
private:
	struct st_NODE
	{
		st_NODE* pNext;
		Type data;
	};
	struct st_TOP
	{
		st_NODE* pNode;
		ULONG64 uniqueKey;
	};


public:
	CFreeList();
	~CFreeList();

	Type* Alloc();
	void Free(Type* pData);

	long GetUseCount() { return _useCount; }
	long GetAllocCount() { return _allocCount; }


private:
	long _useCount;
	long _allocCount;

	st_TOP* _pTop;
};


template<class Type>
inline CFreeList<Type>::CFreeList()
{
	_useCount = 0;
	_allocCount = 0;

	_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), 16);
	_pTop->pNode = nullptr;
	_pTop->uniqueKey = 0;
}


template<class Type>
inline CFreeList<Type>::~CFreeList()
{
	while (_pTop->pNode)
	{
		st_NODE* pNode = _pTop->pNode;
		_pTop->pNode = pNode->pNext;
		delete pNode;
	}

	_aligned_free(_pTop);
}


template<class Type>
inline Type * CFreeList<Type>::Alloc()
{
	InterlockedIncrement(&_useCount);

	st_TOP top;
	top.pNode = _pTop->pNode;
	top.uniqueKey = _pTop->uniqueKey;
	while(1)
	{
		if (nullptr == top.pNode)
		{
			InterlockedIncrement(&_allocCount);
			return &((new st_NODE)->data);
		}
		if (InterlockedCompareExchange128((LONG64*)_pTop, top.uniqueKey + 1, (LONG64)top.pNode->pNext, (LONG64*)&top))
		{
			return &(top.pNode->data);
		}
	}
}


template<class Type>
inline void CFreeList<Type>::Free(Type * pData)
{
	InterlockedDecrement(&_useCount);
	st_NODE* pNode = (st_NODE*)((char*)pData - sizeof(st_NODE*));

	st_TOP top;
	top.uniqueKey = _pTop->uniqueKey;
	top.pNode = _pTop->pNode;
	while(1)
	{
		pNode->pNext = top.pNode;
		if (InterlockedCompareExchange128((LONG64*)_pTop, top.uniqueKey + 1, (LONG64)pNode, (LONG64*)&top))
			return;
	}
}