#pragma once
#include "MemoryPool.h"
#include "MemoryPool_TLS.h"

class CPacket
{
public:
	enum en_DEFINE
	{
		eMAX_SHORT_HEADER_SIZE = 2,
		eMAX_HEADER_SIZE = 2,
		eMAX_PAYLOAD_SIZE = 4096,
		eMAX_BUFFER_SIZE = eMAX_HEADER_SIZE + eMAX_PAYLOAD_SIZE,

		ePUSH_ERR = 0,
		ePOP_ERR = 1
	};
	struct st_ERR_INFO
	{
		int _errType;
		int _wantSize;
		int _possibleSize;

		st_ERR_INFO(int errType, int wantSize, int possibleSize)
		{
			_errType = errType;
			_wantSize = wantSize;
			_possibleSize = possibleSize;
		}
	};

public:
	//	static		CFreeList<CPacket> *pMemoryPool;
	static		CMemoryPool_TLS<CPacket> *pMemoryPool;
	static		CPacket * Alloc();
	static void MemoryPool_Init();

	static __int64		GetUsePool() { return pMemoryPool->GetUseCount(); }
	static __int64		GetAllocPool() { return pMemoryPool->GetAllocCount(); }

	void		addRef();
	void		Free();

public:
	CPacket();
	~CPacket();


public:
	void Clear();

	//	NetServer���� ����ϱ� ���ؼ� friend ó���� �ؾ���
	//	���������� ȣ����� �ʰ� �ϱ� ���ؼ���
	int GetBufferSize() { return _bufferSize; }
	int GetDataSize() { return _dataSize; }
	int GetPacketSize() { return eMAX_HEADER_SIZE + _dataSize; }
	int GetPacketSize_CustomHeader(int iCustomeHeaderSize) { return iCustomeHeaderSize + _dataSize; }
	int GetFreeSize() { return eMAX_PAYLOAD_SIZE - _dataSize; }

	char* GetBufferPtr() { return _buffer; }
	char* GetWritePtr() { return _pWritePos; }
	char* GetReadPtr() { return _pReadPos; }

	void PushData(char* pSrc, int size);
	void PopData(char* pDest, int size);

	void PushData(int size);
	void PopData(int size);

	void SetHeader(char * pHeader);				//		�⺻ 5����Ʈ ��� ����
	void SetHeader_CustomHeader(char * pHeader, int iCustomHeaderSize);		//	���ũ�� ���� �� ����
	void SetHeader_CustomShort(unsigned short Header);		//	2����Ʈ¥�� ��� ����

public:
	//������ �� �� Pop�� �� ���¶��ص� ���� ����� ���� �װͱ����� �������� �ʰ� �׳� _pReadPos��
	//���۽��������� ����� ������ ��������� ��쿡 ���� ����� �� �ִ� ������ �þ�� ������ �ִ�.
	CPacket & operator=(CPacket& srcPacket);


	/*//////////////////////////////////////////////////////////////
	Push
	//////////////////////////////////////////////////////////////*/
	CPacket& operator<<(char value);
	CPacket& operator<<(unsigned char value);

	CPacket& operator<<(short value);
	CPacket& operator<<(unsigned short value);

	CPacket& operator<<(int value);
	CPacket& operator<<(unsigned int value);
	CPacket& operator<<(long value);
	CPacket& operator<<(unsigned long value);
	CPacket& operator<<(float value);

	CPacket& operator<<(__int64 value);
	CPacket& operator<<(double value);



	/*//////////////////////////////////////////////////////////////
	Pop
	//////////////////////////////////////////////////////////////*/
	CPacket& operator >> (char& value);
	CPacket& operator >> (unsigned char& value);

	CPacket& operator >> (short& value);
	CPacket& operator >> (unsigned short& value);

	CPacket& operator >> (int& value);
	CPacket& operator >> (unsigned int& value);
	CPacket& operator >> (long& value);
	CPacket& operator >> (unsigned long& value);
	CPacket& operator >> (float& value);

	CPacket& operator >> (__int64& value);
	CPacket& operator >> (double& value);



private:
	char				_buffer[eMAX_BUFFER_SIZE];
	int					_bufferSize;
	char*				_pEndPos;

	int					_dataSize;
	__int64				_RefCount;

	char*			_pWritePos;
	char*			_pReadPos;

	long			HeaderSetFlag;
};

