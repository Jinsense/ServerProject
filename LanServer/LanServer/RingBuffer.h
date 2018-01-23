#pragma once

class CRingBuffer
{
public:
	CRingBuffer();
	CRingBuffer(int bufferSize);
	~CRingBuffer();
	void Initialize(int bufferSize);


public:
	int GetBufferSize();
	int GetFreeSize();
	int GetUseSize();

	int GetNotBrokenPushSize();
	int GetNotBrokenPopSize();

	int Enqueue(const char* pData, int dataSize);
	int Dequeue(char* pData, int dataSize);

	int Enqueue(int dataSize);
	int Dequeue(int dataSize);

	int Peek(char* pData, int dataSize);

	void Clear() { _front = _rear; }

	char* GetBufferPtr() { return _pBuffer; }
	char* GetWriteBufferPtr() { return &_pBuffer[_rear]; }
	char* GetReadBufferPtr() { return &_pBuffer[_front]; }


public:
	void Lock();
	void Unlock();


private:
	char* _pBuffer;
	int _bufferSize;

	int _front;
	int _rear;

	CRITICAL_SECTION _cs;
	PCRITICAL_SECTION _pCS;
};
