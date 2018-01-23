#include <Windows.h>
#include <string.h>

#include "RingBuffer.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

CRingBuffer::CRingBuffer()
{
	m_pCS = &m_CS;
	InitializeCriticalSection(m_pCS);
	m_iFront = 0;
	m_iRear = 0;
}

CRingBuffer::CRingBuffer(int iBufferSize)
{
	Initialize(iBufferSize);
	m_pCS = &m_CS;
	InitializeCriticalSection(m_pCS);
	m_iFront = 0;
	m_iRear = 0;
}

CRingBuffer::~CRingBuffer()
{
	DeleteCriticalSection(m_pCS);
	delete[] m_pBuffer;
}

void CRingBuffer::Initialize(int iBufferSize)
{
	m_pBuffer = new char[iBufferSize];
	m_iBufferSize = iBufferSize;
}

int CRingBuffer::GetBufferSize()
{
	return m_iBufferSize;
}

int CRingBuffer::GetFreeSize()
{
	int _iFront = m_iFront;

	if (_iFront <= m_iRear)
		return m_iBufferSize - m_iRear + _iFront - 1;
	else
		return _iFront - m_iRear - 1;
}

int CRingBuffer::GetUseSize()
{
	int _iRear = m_iRear;

	if (m_iFront <= _iRear)
		return _iRear - m_iFront;
	else
		return _iRear + m_iBufferSize - m_iFront;
}

int CRingBuffer::GetNotBrokenPushSize()
{
	int _iFront = m_iFront;

	if (_iFront <= m_iRear)
	{
		if (0 == _iFront)
			return m_iBufferSize - m_iRear - 1;
		else
			return m_iBufferSize - m_iRear;
	}
	else
		return _iFront - m_iRear - 1;
}

int CRingBuffer::GetNotBrokenPopSize()
{
	int _iRear = m_iRear;

	if (m_iFront <= _iRear)
		return _iRear - m_iFront;
	else
		return m_iBufferSize - m_iFront;
}

int CRingBuffer::Enqueue(const char *pData, int iDataSize)
{
	EnterCriticalSection(&m_CS);

	int _iDestPos = m_iRear + iDataSize;
	int _iFront = m_iFront;

	if (_iFront <= m_iRear)
	{
		if (_iDestPos < m_iBufferSize)
		{
			memcpy_s(&m_pBuffer[m_iRear], iDataSize, pData, iDataSize);
			m_iRear += iDataSize;
		}
		else
		{
			int _iFirstSize = m_iBufferSize - m_iRear;
			int _iSecondSize = _iDestPos - m_iBufferSize;
			int _iNewRear;

			if (_iFront <= _iSecondSize)
			{
				if (0 == _iFront)
				{
					iDataSize -= _iSecondSize + 1;
					_iSecondSize = 0;
					_iFirstSize--;
					_iNewRear = m_iRear + _iFirstSize;
				}
				else
				{
					_iSecondSize = _iFront - 1;
					iDataSize = _iFirstSize + _iSecondSize;
					_iNewRear = _iSecondSize;
				}
			}
			else
				_iNewRear = _iSecondSize;
			memcpy_s(&m_pBuffer[m_iRear], _iFirstSize, pData, _iFirstSize);
			memcpy_s(m_pBuffer, _iSecondSize, &pData[_iFirstSize], _iSecondSize);
			m_iRear = _iNewRear;
		}
	}
	else
	{
		if (_iFront <= _iDestPos)
			iDataSize -= (_iDestPos - _iFront) + 1;

		memcpy_s(&m_pBuffer[m_iRear], iDataSize, pData, iDataSize);
		m_iRear += iDataSize;
	}

	LeaveCriticalSection(&m_CS);
	return iDataSize;
}

int CRingBuffer::Dequeue(char *pData, int iDataSize)
{
	EnterCriticalSection(&m_CS);

	int _iDsetPos = m_iFront + iDataSize;
	int _iRear = m_iRear;

	if (m_iFront <= _iRear)
	{
		if (_iRear < _iDsetPos)
			iDataSize -= _iDsetPos - _iRear;
		memcpy_s(pData, iDataSize, &m_pBuffer[m_iFront], iDataSize);
		m_iFront += iDataSize;
	}
	else
	{
		if (_iDsetPos < m_iBufferSize)
		{
			memcpy_s(pData, iDataSize, &m_pBuffer[m_iFront], iDataSize);
			m_iFront += iDataSize;
		}
		else
		{
			int _iFirstSize = m_iBufferSize - m_iFront;
			int _iSecondSize = _iDsetPos - m_iBufferSize;

			if (_iRear < _iSecondSize)
			{
				_iSecondSize = _iRear;
				iDataSize = _iFirstSize + _iSecondSize;
			}
			memcpy_s(pData, _iFirstSize, &m_pBuffer[m_iFront], _iFirstSize);
			memcpy_s(&pData[_iFirstSize], _iSecondSize, m_pBuffer, _iSecondSize);
			m_iFront = _iSecondSize;
		}
	}

	LeaveCriticalSection(&m_CS);
	return iDataSize;
}

int CRingBuffer::Enqueue(int iDataSize)
{
	EnterCriticalSection(&m_CS);

	int _iDestPos = m_iRear + iDataSize;
	int _iFront = m_iFront;

	if (_iFront <= m_iRear)
	{
		if (_iDestPos < m_iBufferSize)
			m_iRear += iDataSize;
		else
		{
			int _iFirstSize = m_iBufferSize - m_iRear;
			int _iSecondSize = _iDestPos - m_iBufferSize;
			int _iNewRear;

			if (_iFront <= _iSecondSize)
			{
				if (0 == _iFront)
				{
					iDataSize -= _iSecondSize + 1;
					_iSecondSize = 0;
					_iFirstSize--;
					_iNewRear = m_iRear + _iFirstSize;
				}
				else
				{
					_iSecondSize = _iFront - 1;
					iDataSize = _iFirstSize + _iSecondSize;
					_iNewRear = _iSecondSize;
				}
			}
			else
				_iNewRear = _iSecondSize;

			m_iRear = _iNewRear;
		}
	}
	else
	{
		if (_iFront <= _iDestPos)
			iDataSize -= (_iDestPos - _iFront) + 1;
		m_iRear += iDataSize;
	}

	LeaveCriticalSection(&m_CS);
	return iDataSize;
}

int CRingBuffer::Dequeue(int iDataSize)
{
	EnterCriticalSection(&m_CS);

	int iDestPos = m_iFront + iDataSize;
	int _iRear = m_iRear;

	if (m_iFront <= _iRear)
	{
		if (_iRear < iDestPos)
			iDataSize -= iDestPos - _iRear;

		m_iFront += iDataSize;
	}
	else
	{
		if (iDestPos < m_iBufferSize)
			m_iFront += iDataSize;
		else
		{
			int _iFirstSize = m_iBufferSize - m_iFront;
			int _iSecondSize = iDestPos - m_iBufferSize;

			if (_iRear < _iSecondSize)
			{
				_iSecondSize = _iRear;
				iDataSize = _iFirstSize + _iSecondSize;
			}
			m_iFront = _iSecondSize;
		}
	}

	LeaveCriticalSection(&m_CS);
	return iDataSize;
}

int CRingBuffer::Peek(char *pData, int iDataSize)
{
	iDataSize = min(iDataSize, GetUseSize());
	int _iDestPos = m_iFront + iDataSize;

	if (m_iFront <= m_iRear)
		memcpy_s(pData, iDataSize, &m_pBuffer[m_iFront], iDataSize);
	else
	{
		if (_iDestPos < m_iBufferSize)
			memcpy_s(pData, iDataSize, &m_pBuffer[m_iFront], iDataSize);
		else
		{
			int firstSize = m_iBufferSize - m_iFront;
			int secondSize = _iDestPos - m_iBufferSize;
			memcpy_s(pData, firstSize, &m_pBuffer[m_iFront], firstSize);
			memcpy_s(&pData[firstSize], secondSize, m_pBuffer, secondSize);
		}
	}
	return iDataSize;
}

void CRingBuffer::Lock()
{
	EnterCriticalSection(m_pCS);
}

void CRingBuffer::Unlock()
{
	LeaveCriticalSection(m_pCS);
}
