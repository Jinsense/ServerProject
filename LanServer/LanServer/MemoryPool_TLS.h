#pragma once
#include <Windows.h>
#include <stack>
#include "MemoryPool.h"
using namespace std;

template<class Type>
class CMemoryPool_TLS
{
private:
	enum en_BLOCK
	{
		eNUM_CHUNKBLOCK = 300,
		eNUM_DEFAULT = 10,
	};

	class CChunk
	{
		struct st_BLOCK
		{
			CChunk * pChunkBlock;
			Type data;
		};

	public:
		CChunk() : _pEnd((st_BLOCK*)((char*)_arBlock + sizeof(CChunk*) + (sizeof(st_BLOCK) * eNUM_CHUNKBLOCK)))
		{
			for (auto i = 0; i < eNUM_CHUNKBLOCK; i++)
			{
				_arBlock[i].pChunkBlock = this;
			}
		}
		void Initialize()
		{
			_pTop = (st_BLOCK*)((char*)_arBlock + sizeof(CChunk*));
			_RefCount = eNUM_CHUNKBLOCK;
		}

	private:
		st_BLOCK _arBlock[eNUM_CHUNKBLOCK];
	public:
		const st_BLOCK * const	_pEnd;
		st_BLOCK *				_pTop;

		long _RefCount;
		friend class CMemoryPool_TLS;
	};

public:
	CMemoryPool_TLS()
	{
		TLSIndex = TlsAlloc();
		UseCount = 0;
		AllocCount = eNUM_DEFAULT;
		InitializeCriticalSection(&_cs);
		for (auto i = 0; i < eNUM_DEFAULT; i++)
		{
			CChunk * pChunk = new CChunk;
			_ChunkStack.push(pChunk);
		}

	}
	~CMemoryPool_TLS()
	{
		while (FALSE == _ChunkStack.empty())
		{
			CChunk * pChunk = _ChunkStack.top();
			_ChunkStack.pop();
			delete pChunk;
		}
		TlsFree(TLSIndex);
	}


	Type * Alloc()
	{
		InterlockedIncrement(&UseCount);

		CChunk * pChunk;

		if (TlsGetValue(TLSIndex) == NULL)
		{
			pChunk = ChunkAlloc();
			pChunk->Initialize();
			TlsSetValue(TLSIndex, pChunk);
		}
		else
			pChunk = (CChunk*)TlsGetValue(TLSIndex);

		Type * Packet = (Type*)(pChunk->_pTop);

		pChunk->_pTop = pChunk->_pTop++;

		if (pChunk->_pTop == pChunk->_pEnd)
			TlsSetValue(TLSIndex, NULL);


		return Packet;
	}

	void Free(Type* pData)
	{
		InterlockedDecrement(&UseCount);
		CChunk::st_BLOCK * pBlock = (CChunk::st_BLOCK *)((char*)pData - sizeof(CChunk*));
		CChunk * pChunk = pBlock->pChunkBlock;
		if (0 == InterlockedDecrement(&pChunk->_RefCount))
		{
			ChunkFree(pChunk);
		}
		return;
	}

	CChunk* ChunkAlloc()
	{
		EnterCriticalSection(&_cs);
		CChunk * pChunk;
		if (TRUE == _ChunkStack.empty())
		{
			pChunk = new CChunk;
			InterlockedIncrement(&AllocCount);
		}
		else
		{
			pChunk = _ChunkStack.top();
			_ChunkStack.pop();
		}
		LeaveCriticalSection(&_cs);
		return pChunk;
	}

	void ChunkFree(CChunk* pChunk)
	{
		EnterCriticalSection(&_cs);
		_ChunkStack.push(pChunk);
		LeaveCriticalSection(&_cs);

		return;
	}

	long		GetUseCount() { return UseCount; }
	long		GetAllocCount() { return (AllocCount * eNUM_CHUNKBLOCK); }

private:
	DWORD	TLSIndex;
	long	UseCount;
	long	AllocCount;

	stack<CChunk*> _ChunkStack;
	CRITICAL_SECTION _cs;

	friend class CChunk;
};