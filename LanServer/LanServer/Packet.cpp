#include "Packet.h"
#include "Dump.h"
#include <Windows.h>


//	CFreeList<CPacket>* CPacket::pMemoryPool = NULL;
CMemoryPool_TLS<CPacket>* CPacket::pMemoryPool = NULL;

CPacket::CPacket() :
	_bufferSize(eMAX_BUFFER_SIZE),
	_pEndPos(_buffer + eMAX_BUFFER_SIZE)
{
	ZeroMemory(&_buffer, eMAX_BUFFER_SIZE);
	_dataSize = 0;

	_pWritePos = _buffer + eMAX_HEADER_SIZE;
	_pReadPos = _buffer + eMAX_HEADER_SIZE;

	HeaderSetFlag = FALSE;
	_RefCount = 0;
}


CPacket::~CPacket()
{
}

void CPacket::Clear()
{
	ZeroMemory(&_buffer, eMAX_BUFFER_SIZE);
	_dataSize = 0;
	_bufferSize = eMAX_BUFFER_SIZE;
	_pEndPos = _buffer + eMAX_BUFFER_SIZE;

	_pWritePos = _buffer + eMAX_HEADER_SIZE;
	_pReadPos = _buffer + eMAX_HEADER_SIZE;

	HeaderSetFlag = FALSE;
	_RefCount = 0;
}

CPacket * CPacket::Alloc()
{
	CPacket * pPacket = pMemoryPool->Alloc();
	pPacket->Clear();
	pPacket->addRef();
	return pPacket;
}

void CPacket::Free()
{
	if (0 >= InterlockedDecrement64(&_RefCount))
	{
		if (0 > _RefCount)
			g_CrashDump->Crash();
		pMemoryPool->Free(this);
	}
}

void CPacket::MemoryPool_Init()
{
	//	pMemoryPool = new CFreeList<CPacket>();
	if (pMemoryPool == nullptr)
		pMemoryPool = new CMemoryPool_TLS<CPacket>();
}

void CPacket::addRef()
{
	InterlockedIncrement64(&_RefCount);
}

void CPacket::PushData(char* pSrc, int size)
{
	if (_pEndPos - _pWritePos < size)
		throw st_ERR_INFO(ePUSH_ERR, size, int(_pEndPos - _pWritePos));

	memcpy_s(_pWritePos, size, pSrc, size);
	_pWritePos += size;
	_dataSize += size;
}


void CPacket::PopData(char* pDest, int size)
{
	if (_dataSize < size)
		throw st_ERR_INFO(ePOP_ERR, size, _dataSize);

	memcpy_s(pDest, size, _pReadPos, size);
	_pReadPos += size;
	_dataSize -= size;
}


void CPacket::PushData(int size)
{
	if (_pEndPos - _pWritePos < size)
		throw st_ERR_INFO(ePUSH_ERR, size, int(_pEndPos - _pWritePos));

	_pWritePos += size;
	_dataSize += size;
}


void CPacket::PopData(int size)
{
	if (_dataSize < size)
		throw st_ERR_INFO(ePOP_ERR, size, _dataSize);

	_pReadPos += size;
	_dataSize -= size;
}


void CPacket::SetHeader(char * pHeader)
{
	if (TRUE == InterlockedCompareExchange(&HeaderSetFlag, FALSE, TRUE))
		return;
	memcpy_s(_buffer, eMAX_HEADER_SIZE, pHeader, eMAX_HEADER_SIZE);
}

void CPacket::SetHeader_CustomHeader(char * pHeader, int iCustomHeaderSize)
{
	if (TRUE == InterlockedCompareExchange(&HeaderSetFlag, FALSE, TRUE))
		return;
	int iSize = eMAX_HEADER_SIZE - iCustomHeaderSize;
	memcpy_s(&_buffer[iSize], eMAX_HEADER_SIZE, pHeader, iCustomHeaderSize);
}

void CPacket::SetHeader_CustomShort(unsigned short Header)
{
	if (TRUE == InterlockedCompareExchange(&HeaderSetFlag, FALSE, TRUE))
		return;
	//	memcpy_s(&_buffer[3], eMAX_SHORT_HEADER_SIZE, &Header, eMAX_SHORT_HEADER_SIZE);
	memcpy_s(&_buffer, eMAX_SHORT_HEADER_SIZE, &Header, eMAX_SHORT_HEADER_SIZE);
}

CPacket& CPacket::operator=(CPacket& srcPacket)
{
	_dataSize = srcPacket._dataSize;

	_pWritePos = _buffer + eMAX_HEADER_SIZE + _dataSize;
	_pReadPos = _buffer + eMAX_HEADER_SIZE;

	memcpy_s(_buffer, _dataSize, srcPacket._pReadPos, _dataSize);

	return *this;
}


CPacket& CPacket::operator<<(char value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(unsigned char value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(short value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(unsigned short value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(int value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(unsigned int value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(long value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(unsigned long value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(float value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(__int64 value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator<<(double value)
{
	PushData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (char& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (unsigned char& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (short& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (unsigned short& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (int& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (unsigned int& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (long& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (unsigned long& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (float& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (__int64& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}


CPacket& CPacket::operator >> (double& value)
{
	PopData((char*)&value, sizeof(value));
	return *this;
}