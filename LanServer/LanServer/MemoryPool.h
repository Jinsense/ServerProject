#ifndef _LANSERVER_MEMORY_POOL_H_
#define _LANSERVER_MEMORY_POOL_H_

#include <Windows.h>

template<class Type>
class CFreeList
{
private:
	struct st_NODE
	{
		st_NODE				*pNext;
		Type				Data;

		st_NODE() :
			pNext(nullptr)
		{}
	};
	struct st_TOP
	{
		st_NODE				*pNode;
		unsigned __int64	iCount;

		st_TOP() :
			pNode(nullptr),
			iCount(NULL) {}
	};

public:
	CFreeList();
	~CFreeList();

	Type *Alloc();
	void Free(Type *pData);

	long GetUseCount() { return m_lUseCount; }
	long GetAllocCount() { return m_lAllocCount; }

private:
	long	m_lUseCount;
	long	m_lAllocCount;
	st_TOP	*m_pTop;
};

template<class Type>
inline CFreeList<Type>::CFreeList()
{
	m_lUseCount = 0;
	m_lAllocCount = 0;

	m_pTop = (st_TOP*)_aligned_malloc(sizeof(st_TOP), 16);
	m_pTop->pNode = nullptr;
	m_pTop->iCount = 0;
}

template<class Type>
inline CFreeList<Type>::~CFreeList()
{
	st_NODE *_pNode = m_pTop->pNode;
	while (_pNode)
	{
		st_NODE* _pNext = _pNode->pNext;
		delete _pNode;
		_pNode = _pNext;
		InterlockedDecrement(&m_lUseCount);
	}
	_aligned_free(m_pTop);
}

template<class Type>
inline Type * CFreeList<Type>::Alloc()
{
	InterlockedIncrement(&m_lUseCount);

	st_TOP _Top;
	_Top.pNode = m_pTop->pNode;
	_Top.iCount = m_pTop->iCount;
	for (; ; )
	{
		if (nullptr == _Top.pNode)
		{
			InterlockedIncrement(&m_lAllocCount);
			return &((new st_NODE)->Data);
		}
		if (InterlockedCompareExchange128((LONG64*)m_pTop, _Top.iCount + 1,
			(LONG64)_Top.pNode->pNext, (LONG64*)&_Top))
		{
			return &(_Top.pNode->Data);
		}
	}
}

template<class Type>
inline void CFreeList<Type>::Free(Type *pData)
{
	InterlockedDecrement(&m_lUseCount);
	st_NODE *_pNode = (st_NODE*)((char*)pData - sizeof(st_NODE*));

	st_TOP _Top;
	_Top.iCount = m_pTop->iCount;
	_Top.pNode = m_pTop->pNode;
	for (; ; )
	{
		_pNode->pNext = _Top.pNode;
		if (InterlockedCompareExchange128((LONG64*)m_pTop, _Top.iCount + 1,
			(LONG64)_pNode, (LONG64*)&_Top))
		{
			return;
		}
	}
}

#endif _LANSERVER_MEMORY_POOL_H_