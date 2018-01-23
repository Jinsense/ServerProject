#include <Windows.h>
#include <string.h>
#include "RingBuffer.h"
#define min(a,b) (((a) < (b)) ? (a) : (b))



CRingBuffer::CRingBuffer()
{
	_pCS = &_cs;
	InitializeCriticalSection(_pCS);

	_front = 0;
	_rear = 0;
}


CRingBuffer::CRingBuffer(int bufferSize)
{
	Initialize(bufferSize);

	_pCS = &_cs;
	InitializeCriticalSection(_pCS);

	_front = 0;
	_rear = 0;
}


CRingBuffer::~CRingBuffer()
{
	DeleteCriticalSection(_pCS);

	delete[] _pBuffer;
}


void CRingBuffer::Initialize(int bufferSize)
{
	_pBuffer = new char[bufferSize];
	_bufferSize = bufferSize;
}


int CRingBuffer::GetBufferSize()
{
	return _bufferSize;
}


int CRingBuffer::GetFreeSize()
{
	//현재 Enqueue와는 동시 진행이 불가능한 상태
	int front = _front;

	if (front <= _rear)
		return _bufferSize - _rear + front - 1;
	else
		return front - _rear - 1;
}


int CRingBuffer::GetUseSize()
{
	//현재 Dequeue와는 동시 진행이 불가능한 상태
	int rear = _rear;

	if (_front <= rear)
		return rear - _front;
	else
		return rear + _bufferSize - _front;
}


int CRingBuffer::GetNotBrokenPushSize()
{
	//현재 Enqueue와는 동시 진행이 불가능한 상태
	int front = _front;

	if (front <= _rear)
	{
		if (0 == front)
			return _bufferSize - _rear - 1;
		else
			return _bufferSize - _rear;
	}
	else
		return front - _rear - 1;
}


int CRingBuffer::GetNotBrokenPopSize()
{
	//현재 Dequeue와는 동시 진행이 불가능한 상태
	int rear = _rear;

	if (_front <= rear)
		return rear - _front;
	else
		return _bufferSize - _front;
}


int CRingBuffer::Enqueue(const char* pData, int dataSize)
{
	EnterCriticalSection(&_cs);

	int destPos = _rear + dataSize;
	int front = _front;

	if (front <= _rear)
	{
		if (destPos < _bufferSize)
		{
			memcpy_s(&_pBuffer[_rear], dataSize, pData, dataSize);
			_rear += dataSize;
		}
		else
		{
			int firstSize = _bufferSize - _rear;
			int secondSize = destPos - _bufferSize;
			int newRear;

			//어떤 상황에서도 항상 빈 칸 하나는 남겨놓는다.
			if (front <= secondSize)
			{
				if (0 == front)
				{
					dataSize -= secondSize + 1;
					secondSize = 0;
					firstSize--;

					newRear = _rear + firstSize;
				}
				else
				{
					secondSize = front - 1;
					dataSize = firstSize + secondSize;

					newRear = secondSize;
				}
			}
			else
				newRear = secondSize;

			memcpy_s(&_pBuffer[_rear], firstSize, pData, firstSize);
			memcpy_s(_pBuffer, secondSize, &pData[firstSize], secondSize);

			_rear = newRear;
		}
	}
	else
	{
		//어떤 상황에서도 항상 빈 칸 하나는 남겨놓는다.
		if (front <= destPos)
			dataSize -= (destPos - front) + 1;

		memcpy_s(&_pBuffer[_rear], dataSize, pData, dataSize);
		_rear += dataSize;
	}

	LeaveCriticalSection(&_cs);
	return dataSize;
}


int CRingBuffer::Dequeue(char* pData, int dataSize)
{
	EnterCriticalSection(&_cs);

	int destPos = _front + dataSize;
	int rear = _rear;

	if (_front <= rear)
	{
		if (rear < destPos)
			dataSize -= destPos - rear;

		memcpy_s(pData, dataSize, &_pBuffer[_front], dataSize);
		_front += dataSize;
	}
	else
	{
		if (destPos < _bufferSize)
		{
			memcpy_s(pData, dataSize, &_pBuffer[_front], dataSize);
			_front += dataSize;
		}
		else
		{
			int firstSize = _bufferSize - _front;
			int secondSize = destPos - _bufferSize;

			if (rear < secondSize)
			{
				secondSize = rear;
				dataSize = firstSize + secondSize;
			}

			memcpy_s(pData, firstSize, &_pBuffer[_front], firstSize);
			memcpy_s(&pData[firstSize], secondSize, _pBuffer, secondSize);

			_front = secondSize;
		}
	}

	LeaveCriticalSection(&_cs);
	return dataSize;
}


int CRingBuffer::Enqueue(int dataSize)
{
	EnterCriticalSection(&_cs);

	int destPos = _rear + dataSize;
	int front = _front;

	if (front <= _rear)
	{
		if (destPos < _bufferSize)
			_rear += dataSize;
		else
		{
			int firstSize = _bufferSize - _rear;
			int secondSize = destPos - _bufferSize;
			int newRear;

			//어떤 상황에서도 항상 빈 칸 하나는 남겨놓는다.
			if (front <= secondSize)
			{
				if (0 == front)
				{
					dataSize -= secondSize + 1;
					secondSize = 0;
					firstSize--;

					newRear = _rear + firstSize;
				}
				else
				{
					secondSize = front - 1;
					dataSize = firstSize + secondSize;

					newRear = secondSize;
				}
			}
			else
				newRear = secondSize;

			_rear = newRear;
		}
	}
	else
	{
		//어떤 상황에서도 항상 빈 칸 하나는 남겨놓는다.
		if (front <= destPos)
			dataSize -= (destPos - front) + 1;

		_rear += dataSize;
	}

	LeaveCriticalSection(&_cs);
	return dataSize;
}


int CRingBuffer::Dequeue(int dataSize)
{
	EnterCriticalSection(&_cs);

	int destPos = _front + dataSize;
	int rear = _rear;

	if (_front <= rear)
	{
		if (rear < destPos)
			dataSize -= destPos - rear;

		_front += dataSize;
	}
	else
	{
		if (destPos < _bufferSize)
			_front += dataSize;
		else
		{
			int firstSize = _bufferSize - _front;
			int secondSize = destPos - _bufferSize;

			if (rear < secondSize)
			{
				secondSize = rear;
				dataSize = firstSize + secondSize;
			}

			_front = secondSize;
		}
	}

	LeaveCriticalSection(&_cs);
	return dataSize;
}


int CRingBuffer::Peek(char* pData, int dataSize)
{
	dataSize = min(dataSize, GetUseSize());

	int destPos = _front + dataSize;

	if (_front <= _rear)
		memcpy_s(pData, dataSize, &_pBuffer[_front], dataSize);
	else
	{
		if (destPos < _bufferSize)
			memcpy_s(pData, dataSize, &_pBuffer[_front], dataSize);
		else
		{
			int firstSize = _bufferSize - _front;
			int secondSize = destPos - _bufferSize;

			memcpy_s(pData, firstSize, &_pBuffer[_front], firstSize);
			memcpy_s(&pData[firstSize], secondSize, _pBuffer, secondSize);
		}
	}

	return dataSize;
}


void CRingBuffer::Lock()
{
	EnterCriticalSection(_pCS);
}


void CRingBuffer::Unlock()
{
	LeaveCriticalSection(_pCS);
}
