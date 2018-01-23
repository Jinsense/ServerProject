#ifndef _LANSERVER_MEMORYPOOL_LFSTACK_H_
#define _LANSERVER_MEMORYPOOL_LFSTACK_H_
#include "MemoryPool.h"


template<class Type>
class CLockFreeStack
{
	struct st_NODE
	{
		st_NODE* pNext;
		Type data;
	};
	struct st_TOP
	{
		st_NODE* pNode;
		LONG64 uniqueKey;
	};


public:
	CLockFreeStack();
	~CLockFreeStack();

	void Push(Type data);
	void Pop(Type* pData);

	LONG GetUseCount();


private:
	LONG * _pUseCount;
	st_TOP* _pTop;

	CFreeList<st_NODE> _freeList;
};


template<class Type>
inline CLockFreeStack<Type>::CLockFreeStack()
{
	_pUseCount = (LONG*)_aligned_malloc(sizeof(LONG), 4);
	*_pUseCount = 0;

	_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), 16);
	_pTop->pNode = nullptr;
	_pTop->uniqueKey = 0;
}


template<class Type>
inline CLockFreeStack<Type>::~CLockFreeStack()
{
	st_NODE* pNode = _pTop->pNode;
	while (pNode)
	{
		st_NODE* pNext = pNode->pNext;
		_freeList.Free(pNode);
		pNode = pNext;

		(*_pUseCount)--;
	}

	_aligned_free(_pTop);
	_aligned_free(_pUseCount);
}


template<class Type>
inline void CLockFreeStack<Type>::Push(Type data)
{
	st_NODE* pNode = _freeList.Alloc();
	pNode->data = data;

	st_TOP top;
	top.pNode = _pTop->pNode;
	top.uniqueKey = _pTop->uniqueKey;
	while(1)
	{
		pNode->pNext = top.pNode;
		if (InterlockedCompareExchange128((LONG64*)_pTop, top.uniqueKey + 1, (LONG64)pNode, (LONG64*)&top))
		{
			InterlockedIncrement(_pUseCount);
			return;
		}
	}
}


template<class Type>
inline void CLockFreeStack<Type>::Pop(Type * pData)
{
	if (InterlockedDecrement(_pUseCount) < 0)
	{
		InterlockedIncrement(_pUseCount);
		*pData = nullptr;
		return;
	}

	st_TOP top;
	top.pNode = _pTop->pNode;
	top.uniqueKey = _pTop->uniqueKey;
	while(1)
	{
		if (InterlockedCompareExchange128((LONG64*)_pTop, top.uniqueKey + 1, (LONG64)top.pNode->pNext, (LONG64*)&top))
		{
			*pData = top.pNode->data;
			_freeList.Free(top.pNode);
			return;
		}
	}
}


template<class Type>
inline long CLockFreeStack<Type>::GetUseCount()
{
	return *_pUseCount;
}

#endif _LANSERVER_MEMORYPOOL_LFSTACK_H_