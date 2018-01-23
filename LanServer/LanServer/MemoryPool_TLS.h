#ifndef _LANSERVER_MEMORY_TLS_H_
#define _LANSERVER_MEMORY_TLS_H_

#include <Windows.h>
#include <stack>

#include "MemoryPool.h"

using namespace std;

template<class Type>
class CMemoryPool_TLS
{
private:
	enum class en_BLOCK
	{
		CHUNKBLOCK = 300,
		DEFAULT = 10,
	};

	class CChunk
	{
		struct st_BLOCK
		{
			CChunk *pChunkBlock;
			Type Data;

			st_BLOCK() :
				pChunkBlock(nullptr)
			{}
		};

	public:
		CChunk() : m_pEnd((st_BLOCK*)((char*)m_Block + sizeof(CChunk*)
			+ (sizeof(st_BLOCK) * static_cast<int>(en_BLOCK::CHUNKBLOCK))))
		{
			for (int i = 0; i < static_cast<int>(en_BLOCK::CHUNKBLOCK); i++)
			{
				m_Block[i].pChunkBlock = this;
			}
		}
		void Initialize()
		{
			m_pTop = (st_BLOCK*)((char*)m_Block + sizeof(CChunk*));
			m_lRefCount = static_cast<int>(en_BLOCK::CHUNKBLOCK);
		}

	public:
		const st_BLOCK	*const	m_pEnd;
		st_BLOCK				*m_pTop;
		long					m_lRefCount;

	private:
		st_BLOCK m_Block[en_BLOCK::CHUNKBLOCK];

		friend class CMemoryPool_TLS;
	};

public:
	CMemoryPool_TLS()
	{
		m_dwTlsIndex = TlsAlloc();
		m_lUseCount = 0;
		m_lAllocCount = static_cast<int>(en_BLOCK::DEFAULT);
		InitializeCriticalSection(&m_CS);
		for (int i = 0; i < static_cast<int>(en_BLOCK::DEFAULT); i++)
		{
			CChunk *_pChunk = new CChunk;
			m_ChunkStack.push(_pChunk);
		}
	}
	~CMemoryPool_TLS()
	{
		while (false == m_ChunkStack.empty())
		{
			CChunk *_pChunk = m_ChunkStack.top();
			m_ChunkStack.pop();
			delete _pChunk;
		}
		TlsFree(m_dwTlsIndex);
	}

	Type * Alloc()
	{
		InterlockedIncrement(&m_lUseCount);

		CChunk *_pChunk;

		if (TlsGetValue(m_dwTlsIndex) == NULL)
		{
			_pChunk = ChunkAlloc();
			_pChunk->Initialize();
			TlsSetValue(m_dwTlsIndex, _pChunk);
		}
		else
			_pChunk = (CChunk*)TlsGetValue(m_dwTlsIndex);

		Type *_pPacket = (Type*)(_pChunk->m_pTop);
		_pChunk->m_pTop = _pChunk->m_pTop++;

		if (_pChunk->m_pTop == _pChunk->m_pEnd)
			TlsSetValue(m_dwTlsIndex, NULL);

		return _pPacket;
	}

	void Free(Type *pData)
	{
		InterlockedDecrement(&m_lUseCount);

		CChunk::st_BLOCK *_pBlock = (CChunk::st_BLOCK *)((char*)pData - sizeof(CChunk*));
		CChunk *_pChunk = _pBlock->pChunkBlock;

		if (0 == InterlockedDecrement(&_pChunk->m_lRefCount))
		{
			ChunkFree(_pChunk);
		}
		return;
	}

	CChunk* ChunkAlloc()
	{
		EnterCriticalSection(&m_CS);
		CChunk *_pChunk;
		if (true == m_ChunkStack.empty())
		{
			_pChunk = new CChunk;
			InterlockedIncrement(&m_lAllocCount);
		}
		else
		{
			_pChunk = m_ChunkStack.top();
			m_ChunkStack.pop();
		}
		LeaveCriticalSection(&m_CS);
		return _pChunk;
	}

	void ChunkFree(CChunk *pChunk)
	{
		EnterCriticalSection(&m_CS);
		m_ChunkStack.push(pChunk);
		LeaveCriticalSection(&m_CS);
		return;
	}

	long	GetUseCount() { return m_lUseCount; }
	long	GetAllocCount()
	{
		return (m_lAllocCount *
			static_cast<int>(en_BLOCK::CHUNKBLOCK));
	}

private:
	DWORD	m_dwTlsIndex;
	long	m_lUseCount;
	long	m_lAllocCount;

	std::stack<CChunk*> m_ChunkStack;
	CRITICAL_SECTION m_CS;

	friend class CChunk;
};

#endif _LANSERVER_MEMORY_TLS_H_