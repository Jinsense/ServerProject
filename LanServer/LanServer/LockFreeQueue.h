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


//���� data�� ������ ���������� �����ϴ�. ���������� ��� Interlocked���� data�� �ٲ�����ϰԲ� �ڵ带 �����ؾ� ������ ���ŷο��� �ִ�.
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
		st_NODE* pNext = front.pNode->pNext;   //�ݵ�� �̷��� ���������� �޾Ƴ��� null�˻縦 �ؾ� ũ���� ���ɼ��� ���� �� �ִ�.
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