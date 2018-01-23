#ifndef _LANSERVER_NETWORK_RINGBUFFER_H_
#define _LANSERVER_NETWORK_RINGBUFFER_H_


class CRingBuffer
{
public:
	CRingBuffer();
	CRingBuffer(int iBufferSize);
	~CRingBuffer();
public:
	void	Initialize(int iBufferSize);
	void	Lock();
	void	Unlock();
	void	Clear() { m_iFront = m_iRear; }
	char*	GetBufferPtr() { return m_pBuffer; }
	char*	GetWriteBufferPtr() { return &m_pBuffer[m_iRear]; }
	char*	GetReadBufferPtr() { return &m_pBuffer[m_iFront]; }
	int		GetBufferSize();
	int		GetFreeSize();
	int		GetUseSize();
	int		GetNotBrokenPushSize();
	int		GetNotBrokenPopSize();
	int		Enqueue(const char *pData, int iDataSize);
	int		Dequeue(char *pData, int iDataSize);
	int		Enqueue(int iDataSize);
	int		Dequeue(int iDataSize);
	int		Peek(char *pData, int iDataSize);

private:
	char	*m_pBuffer;
	int		m_iBufferSize;
	int		m_iFront;
	int		m_iRear;
	CRITICAL_SECTION	m_CS;
	PCRITICAL_SECTION	m_pCS;
};

#endif _LANSERVER_NETWORK_RINGBUFFER_H_