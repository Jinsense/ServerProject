#ifndef _LANSERVER_IOCP_LANSERVER_H_
#define _LANSERVER_IOCP_LANSERVER_H_

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include "Log.h"
#include "Packet.h"
#include "RingBuffer.h"
#include "Dump.h"
#include "MemoryPool.h"
#include "MemoryPool_TLS.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"

#define	df_SERVERPORT			6000
#define df_MAX_WORKER_THREAD	10
#define df_MAX_CLINET_NUM		2000
#define df_MAX_WSABUF_NUM		100
#define df_MAX_QUEUE_SIZE		10000
#define df_HEADER_SIZE			2

#define	df_SET_INDEX(Index, SessionKey)		Index = Index << 48; SessionKey = Index | SessionKey;
#define df_GET_INDEX(Index, SessionKey)		Index = SessionKey >> 48;

typedef struct st_SessionInfo
{
	//	접속자 정보를 전달해줄 구조체
	//	전달해야할 정보가 있을 경우 추가
	st_SessionInfo() : sessionkey(NULL) {}

	ULONG64 sessionkey;
}INFO;

typedef struct st_ReleaseCompare
{
	st_ReleaseCompare() : iocount(0), releaseflag(false) {}

	long iocount;
	long releaseflag;
}COMPARE;

typedef struct st_Session
{
	st_Session() : recvqueue(df_MAX_QUEUE_SIZE), packetqueue(df_MAX_QUEUE_SIZE),
		iocount(0), sendflag(true) {}

	long		iocount;
	long		sendflag;
	long		sendcount;
	bool		bLoginflag;
	bool		bRelease;
	ULONG64		sessionkey;
	OVERLAPPED	sendover;
	OVERLAPPED	recvover;
	CRingBuffer	recvqueue;
	CRingBuffer packetqueue;
	CLockFreeQueue<CPacket*> sendqueue;
	SOCKET		sock;
	INFO		info;
}SESSION;

class CLanServer
{
private:
	CLanServer();
	~CLanServer();

protected:
	void	Disconnect(ULONG64 sessionkey);
	virtual void	OnClientJoin(INFO *pInfo) = 0;
	virtual void	OnClientLeave(ULONG64 sessionkey) = 0;
	virtual void	OnConnectionRequest(WCHAR *pClientIP, int port) = 0;
	virtual void	OnError(int errorcode, WCHAR *pError) = 0;

	bool	ServerStart(WCHAR *pOpenIP, int port, int maxworkerthread,
				bool bNodelay, int maxsession);
	bool	ServerStop();
	bool	SendPacket(ULONG64 sessionkey, CPacket *pPacket);
	long	GetClientCount() { return _connectclient; }
	bool	GetShutdownMode() { return _bShutdown; }
	bool	GetWhiteIPMode() { return _bWhiteipmode; }
	bool	GetMonitorMode() { return _bMonitorflag; }
	bool	SetShutdownMode(bool bFlag);
	bool	SetWhiteIPMode(bool bFlag) { _bWhiteipmode = bFlag; return _bWhiteipmode; }
	bool	SetMonitorMode(bool bFlag) { _bMonitorflag = bFlag; return _bMonitorflag; }

	SESSION*	SessionAcquireLock(ULONG64 sessionkey);
	void	SessionAcquireFree(SESSION *pSession);

private:
	bool	ServerInit();
	bool	ClientShutdown(SESSION *pSession);
	bool	ClientRelease(SESSION *pSession);

	void	PutIndex(unsigned int Index);
	void	WorkerThreadUpdate();
	void	AcceptThreadUpdate();
	void	MonitorThreadUpdate();
	void	StartRecvPost(SESSION *pSession);
	void	RecvPost(SESSION *pSession);
	void	SendPost(SESSION *pSession);
	void	CompleteRecv(SESSION *pSession, DWORD dwTransfered);
	void	CompleteSend(SESSION *pSession, DWORD dwTransfered);
	bool	OnRecv(SESSION *pSession, CPacket *pPacket);
	unsigned int* GetIndex();

	static unsigned int WINAPI WorkerThread(LPVOID arg)
	{
		CLanServer *pWorker = (CLanServer*)arg;
		if (pWorker == NULL)
		{
			wprintf(L"[LanServer :: WorkerThread] Init Error\n");
			return false;
		}
		pWorker->WorkerThreadUpdate();
		return true;
	}

	static unsigned int WINAPI AcceptThread(LPVOID arg)
	{
		CLanServer *pAccept = (CLanServer*)arg;
		if (pAccept = NULL)
		{
			wprintf(L"[LanServer :: AcceptThread] Init Error\n");
			return false;
		}
		pAccept->AcceptThreadUpdate();
		return true;
	}

	static unsigned int WINAPI MonitorThread(LPVOID arg)
	{
		CLanServer *pMonitor = (CLanServer*)arg;
		if (pMonitor == NULL)
		{
			wprintf(L"[LanServer :: MonitorThread] Init Error\n");
			return false;
		}
		pMonitor->MonitorThreadUpdate();
		return true;
	}

private:
	HANDLE	_hIOCP;
	HANDLE	_hWorkerThread[50];
	HANDLE	_hAcceptThread;
	HANDLE	_hMonitorThread;
	HANDLE	_hAllThread[100];

	CLockFreeStack<unsigned int*> _sessionstack;
	COMPARE *_pCompare;
	SESSION *_pSessionarray;
	SOCKET	_listensock;
	CRITICAL_SECTION _sessioncs;
	
	bool	_bWhiteipmode;
	bool	_bShutdown;
	bool	_bMonitorflag;

	unsigned int	*_pIndex;
	long	_allthreadcnt;
	long	_sessionkeycnt;
	long	_accepttps;
	long	_accepttotal;
	long	_recvpackettps;
	long	_sendpackettps;
	long	_connectclient;
};

#endif _LANSERVER_IOCP_LANSERVER_H_