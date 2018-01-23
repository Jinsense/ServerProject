#ifndef _LANSERVER_NETWORK_PACKET_H_
#define _LANSERVER_NETWORK_PACKET_H_

#include "MemoryPool.h"
#include "MemoryPool_TLS.h"

class CPacket
{
public:
	enum class en_PACKETDEFINE
	{
		PUSH_ERR = 0,
		POP_ERR = 1,
		SHORT_HEADER_SIZE = 2,
		HEADER_SIZE = 2,
		PAYLOAD_SIZE = 4096,
		BUFFER_SIZE = HEADER_SIZE + PAYLOAD_SIZE,
	};

	struct st_ERR_INFO
	{
		int iErrType;
		int iRequestSize;
		int iCurrentSize;

		st_ERR_INFO(int _iErrType, int _iRequestSize, int _iCurrentSize)
		{
			iErrType = _iErrType;
			iRequestSize = _iRequestSize;
			iCurrentSize = _iCurrentSize;
		}
	};

public:
	CPacket();
	~CPacket();

	static CPacket*	Alloc();
	static void		MemoryPoolInit();
	static __int64	GetUsePool() { return m_pMemoryPool->GetUseCount(); }
	static __int64	GetAllocPool() { return m_pMemoryPool->GetAllocCount(); }

	void	AddRef();
	void	Free();
	void	Clear();
	void	PushData(char *pSrc, int iSize);
	void	PopData(char *pDest, int iSize);
	void	PushData(int iSize);
	void	PopData(int iSize);
	void	SetHeader(char * pHeader);
	void	SetHeader_CustomHeader(char *pHeader, int iCustomHeaderSize);
	void	SetHeader_CustomShort(unsigned short shHeader);
	char*	GetBufferPtr() { return m_chBuffer; }
	char*	GetWritePtr() { return m_pWritePos; }
	char*	GetReadPtr() { return m_pReadPos; }
	int		GetBufferSize() { return m_iBufferSize; }
	int		GetDataSize() { return m_iDataSize; }
	int		GetPacketSize()
	{
		return static_cast<int>(en_PACKETDEFINE::HEADER_SIZE) + m_iDataSize;
	}
	int		GetPacketSize_CustomHeader(int iCustomeHeaderSize)
	{
		return iCustomeHeaderSize + m_iDataSize;
	}
	int		GetFreeSize()
	{
		return static_cast<int>(en_PACKETDEFINE::PAYLOAD_SIZE) - m_iDataSize;
	}

public:
	CPacket & operator=(CPacket &Packet);

	CPacket& operator<<(char Value);
	CPacket& operator<<(unsigned char Value);
	CPacket& operator<<(short Value);
	CPacket& operator<<(unsigned short Value);
	CPacket& operator<<(int Value);
	CPacket& operator<<(unsigned int Value);
	CPacket& operator<<(long Value);
	CPacket& operator<<(unsigned long Value);
	CPacket& operator<<(float Value);
	CPacket& operator<<(__int64 Value);
	CPacket& operator<<(double Value);

	CPacket& operator >> (char& Value);
	CPacket& operator >> (unsigned char& Value);
	CPacket& operator >> (short& Value);
	CPacket& operator >> (unsigned short& Value);
	CPacket& operator >> (int& Value);
	CPacket& operator >> (unsigned int& Value);
	CPacket& operator >> (long& Value);
	CPacket& operator >> (unsigned long& Value);
	CPacket& operator >> (float& Value);
	CPacket& operator >> (__int64& Value);
	CPacket& operator >> (double& Value);

public:
	static		CMemoryPool_TLS<CPacket> *m_pMemoryPool;

private:
	char		m_chBuffer[static_cast<int>(en_PACKETDEFINE::BUFFER_SIZE)];
	char		*m_pEndPos;
	char		*m_pWritePos;
	char		*m_pReadPos;
	int			m_iBufferSize;
	int			m_iDataSize;
	__int64		m_iRefCount;
	long		m_lHeaderSetFlag;
};

#endif _LANSERVER_NETWORK_PACKET_H_