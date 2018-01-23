#pragma once
#include <Windows.h>
#include "MemoryPool.h"


template<class Type>
class CLockFreeQueue
{
	struct st_NODE
	{
		st_NODE* pNext;
		Type data;
	};
	struct st_TIP
	{
		st_NODE* pNode;
		LONG64 workCount;
	};


public:
	CLockFreeQueue();
	~CLockFreeQueue();

	void Enqueue(Type data);
	BOOL Dequeue(Type* pData);

	LONG GetUseCount();


private:
	LONG _useCount;

	st_TIP* _pFront;
	st_TIP* _pRear;

	CFreeList<st_NODE> _freeList;
};


template<class Type>
inline CLockFreeQueue<Type>::CLockFreeQueue()
{
	_useCount = 0;

	_pFront = (st_TIP*)_aligned_malloc(sizeof(st_TIP), 16);
	_pFront->pNode = _freeList.Alloc();
	_pFront->pNode->pNext = nullptr;
	_pFront->workCount = 0;

	_pRear = (st_TIP*)_aligned_malloc(sizeof(st_TIP), 16);
	_pRear->pNode = _pFront->pNode;
	_pRear->workCount = 0;
}


template<class Type>
inline CLockFreeQueue<Type>::~CLockFreeQueue()
{
}


template<class Type>
inline void CLockFreeQueue<Type>::Enqueue(Type data)
{
	st_NODE* pNewNode = _freeList.Alloc();
	pNewNode->pNext = nullptr;
	pNewNode->data = data;

	st_TIP rear;
	rear.pNode = _pRear->pNode;
	rear.workCount = _pRear->workCount;
	while (1)
	{
		if (nullptr == rear.pNode->pNext)
		{
			if (nullptr == InterlockedCompareExchangePointer((PVOID*)&rear.pNode->pNext, pNewNode, nullptr))
			{
				InterlockedCompareExchange128((LONG64*)_pRear, rear.workCount + 1, (LONG64)pNewNode, (LONG64*)&rear);
				break;
			}
		}

		InterlockedCompareExchange128((LONG64*)_pRear, rear.workCount + 1, (LONG64)rear.pNode->pNext, (LONG64*)&rear);
	}

	InterlockedIncrement(&_useCount);
}


//지금 data는 무조건 지역변수만 가능하다. 전역변수의 경우 Interlocked으로 data를 바꿔줘야하게끔 코드를 수정해야 안전한 번거로움이 있다.
template<class Type>
inline BOOL CLockFreeQueue<Type>::Dequeue(Type * pData)
{
	if (InterlockedDecrement(&_useCount) < 0)
	{
		InterlockedIncrement(&_useCount);
		return FALSE;
	}

	st_TIP front;
	front.pNode = _pFront->pNode;
	front.workCount = _pFront->workCount;
	while (1)
	{
		st_NODE* pNext = front.pNode->pNext;   //반드시 이렇게 지역변수로 받아놓고 null검사를 해야 크래쉬 가능성을 없앨 수 있다.
		if (nullptr == pNext)
		{
			front.pNode = _pFront->pNode;
			front.workCount = _pFront->workCount;
			continue;
		}

		*pData = pNext->data;
		if (InterlockedCompareExchange128((LONG64*)_pFront, front.workCount + 1, (LONG64)front.pNode->pNext, (LONG64*)&front))
		{
			_freeList.Free(front.pNode);
			break;
		}
	}

	return TRUE;
}


template<class Type>
inline LONG CLockFreeQueue<Type>::GetUseCount()
{
	return _useCount;
}