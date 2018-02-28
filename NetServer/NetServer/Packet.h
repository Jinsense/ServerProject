#ifndef _NETSERVER_NETWORK_PACKET_H_
#define _NETSERVER_NETWORK_PACKET_H_

#include "MemoryPool.h"

class CPacket
{
public:
	enum class en_PACKETDEFINE
	{
		PUSH_ERR = 0,
		POP_ERR = 1,
		SHORT_HEADER_SIZE = 2,
		HEADER_SIZE = 5,
		PAYLOAD_SIZE = 2048,
		BUFFER_SIZE = HEADER_SIZE + PAYLOAD_SIZE,

		PACKET_CODE = 119,
		PACKET_KEY1 = 50,
		PACKET_KEY2 = 132,
	};

#pragma pack(push, 1)   
	struct st_PACKET_HEADER
	{
		BYTE	byCode;
		WORD	shLen;
		BYTE	RandKey;
		BYTE	CheckSum;
		st_PACKET_HEADER() :
			byCode(static_cast<int>(en_PACKETDEFINE::PACKET_CODE)), RandKey(rand() % 255) {}
	};
#pragma pack(pop)

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
	void	EnCode();
	bool	DeCode(st_PACKET_HEADER * pInHeader);
	int		GetPacketLen() { return m_chBuffer[1]; }
public:
	CPacket& operator=(CPacket &Packet);

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
	static		CMemoryPool<CPacket> *m_pMemoryPool;

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

#endif _NETSERVER_NETWORK_PACKET_H_