#include <WinSock2.h>
#include <Windows.h>
#include <wchar.h>
#include <WS2tcpip.h>
#include <process.h>
#include <time.h>
#include <iostream>

#include "LanServer.h"

using namespace std;

CLanServer::CLanServer()
{
	_pCompare = nullptr;
	_pSessionarray = nullptr;
	_pIndex = nullptr;
	_listensock = INVALID_SOCKET;

	_bWhiteipmode = false;
	_bShutdown = false;
	_bMonitorflag = true;
	_allthreadcnt = 0;
	_sessionkeycnt = 1;
	_accepttps = 0;
	_accepttotal = 0;
	_recvpackettps = 0;
	_sendpackettps = 0;
	_connectclient = 0;
}

CLanServer::~CLanServer()
{

}

void CLanServer::Disconnect(ULONG64 sessionkey)
{
	ULONG64 Index = sessionkey >> 48;
	SESSION *pSession = &_pSessionarray[Index];

	InterlockedIncrement(&pSession->iocount);

	if (true == pSession->bRelease || sessionkey != pSession->sessionkey)
	{
		if (0 == InterlockedDecrement(&(pSession->iocount)))
			ClientRelease(pSession);
		return;
	}
	shutdown(pSession->sock, SD_BOTH);

	if (0 == InterlockedDecrement(&(pSession->iocount)))
		ClientRelease(pSession);

	return;
}

bool CLanServer::ServerStart(WCHAR *pOpenIP, int port, int maxworkerthread,
	bool bNodelay, int maxsession)
{
	wprintf(L"[Server :: ServerStart] Start\n");

	int retval = 0;
	setlocale(LC_ALL, "Korean");
	InitializeCriticalSection(&_sessioncs);

	CPacket::MemoryPoolInit();

	_pCompare = (COMPARE*)_aligned_malloc(sizeof(COMPARE), 16);
	_pCompare->iocount = 0;
	_pCompare->releaseflag = false;

	_pSessionarray = new SESSION[maxsession];

	for (int i = 0; i < maxsession; i++)
	{
		_pSessionarray[i].iocount = 0;
		_pSessionarray[i].sock = INVALID_SOCKET;
		_pSessionarray[i].sendcount = 0;
		_pSessionarray[i].sessionkey = NULL;
		_pSessionarray[i].sendflag = true;
		_pSessionarray[i].bLoginflag = false;
		_pSessionarray[i].bRelease = false;
	}

	_pIndex = new unsigned int[maxsession];

	for (unsigned int i = 0; i < maxsession; i++)
	{
		_pIndex[i] = i;
		_sessionstack.Push(&_pIndex[i]);
	}

	if ((retval = ServerInit()) == false)
		return false;

	struct sockaddr_in server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	InetPton(AF_INET, pOpenIP, &server_addr.sin_addr);
	server_addr.sin_port = htons(port);

	retval = ::bind(_listensock, (sockaddr*)&server_addr, sizeof(server_addr));
	if (retval == SOCKET_ERROR)
	{
		wprintf(L"[Server :: ServerStart] bind Error\n");
		return false;
	}

	retval = listen(_listensock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		wprintf(L"[Server :: ServerStart] listen Error\n");
		return false;
	}

	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread,
		(LPVOID)this, 0, NULL);
	_hAllThread[_allthreadcnt++] = _hAcceptThread;
	wprintf(L"[Server :: ServerStart] AcceptThread Creat\n");

	for (int i = 0; i < maxworkerthread; i++)
	{
		_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread,
			(LPVOID)this, 0, NULL);
		_hAllThread[_allthreadcnt++] = _hWorkerThread[i];
	}
	wprintf(L"[Server :: ServerStart] WorkerThread Create\n");

	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, &MonitorThread,
		(LPVOID)this, 0, NULL);
	_hAllThread[_allthreadcnt++] = _hMonitorThread;

	wprintf(L"[Server :: ServerStart]	MonitorThread Create\n");
	wprintf(L"[Server :: ServerStart]	Complete\n");
	return true;
}

bool CLanServer::ServerStop()
{
	//	ServerStart에서 동적할당 해준 모든 멤버변수에 대해 반환작업
	//	그 외에 서버 재가동을 위해 필요한 셋팅 추가
	//	Accept, Worker, Monitor 스레드 중단
	wprintf(L"[Server :: ServerStop]	Start\n");

	_bShutdown = true;

	PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
	WaitForMultipleObjects(_allthreadcnt, _hAllThread, true, INFINITE);

	while (0 != _sessionstack.GetUseCount())
	{
		unsigned int *pIndex = nullptr;
		_sessionstack.Pop(&pIndex);
	}

	_aligned_free(_pCompare);
	delete _pSessionarray;
	delete _pIndex;

	WSACleanup();

	_listensock = INVALID_SOCKET;

	_bWhiteipmode = false;
	_allthreadcnt = 0;
	_sessionkeycnt = 1;
	_accepttps = 0;
	_accepttotal = 0;
	_recvpackettps = 0;
	_sendpackettps = 0;
	_connectclient = 0;

	wprintf(L"[Server :: ServerStop]	Complete\n");
	return true;
}

bool CLanServer::SendPacket(ULONG64 sessionkey, CPacket *pPacket)
{
	ULONG64 index = df_GET_INDEX(index, sessionkey);

	if (1 == InterlockedIncrement(&_pSessionarray[index].iocount))
	{
		if (0 == InterlockedDecrement(&_pSessionarray[index].iocount))
			ClientRelease(&_pSessionarray[index]);
		return false;
	}

	if (true == _pSessionarray[index].bRelease)
	{
		if (0 == InterlockedDecrement(&_pSessionarray[index].iocount))
			ClientRelease(&_pSessionarray[index]);
		return false;
	}

	if (_pSessionarray[index].sessionkey == sessionkey)
	{
		if (_pSessionarray[index].bLoginflag != true)
			return false;
		_sendpackettps++;
		pPacket->AddRef();
		pPacket->SetHeader_CustomShort(pPacket->GetDataSize());
		_pSessionarray[index].sendqueue.Enqueue(pPacket);

		if (0 == InterlockedDecrement(&_pSessionarray[index].iocount))
		{
			ClientRelease(&_pSessionarray[index]);
			return false;
		}
		SendPost(&_pSessionarray[index]);
		return true;
	}
	else
	{
		if (0 == InterlockedDecrement(&_pSessionarray[index].iocount))
			ClientRelease(&_pSessionarray[index]);
		return false;		
	}
	return true;
}

bool CLanServer::SetShutdownMode(bool bFlag)
{
	if (false == bFlag)
	{
		_bShutdown = false;
		return false;
	}

	ServerStop();
	return true;
}

SESSION* CLanServer::SessionAcquireLock(ULONG64 sessionkey)
{
	ULONG64 index = sessionkey >> 48;
	SESSION *pSession = &_pSessionarray[index];
	InterlockedIncrement(&pSession->iocount);

	return pSession;
}

void CLanServer::SessionAcquireFree(SESSION *pSession)
{
	LONG retval = InterlockedDecrement(&pSession->iocount);
	if (0 >= retval)
	{
		ClientRelease(pSession);
	}
}

bool CLanServer::ServerInit()
{
	WSADATA data;
	int retval = WSAStartup(MAKEWORD(2, 2), &data);
	if (0 != retval)
	{
		wprintf(L"[Server :: ServerInit]	WSAStartup Error\n");
		return false;
	}

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (NULL == _hIOCP)
	{
		wprintf(L"[Server :: ServerInit]	IOCP Init Error\n");
		return false;
	}

	_listensock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _listensock)
	{
		wprintf(L"[Server :: ServerInit]	Listen Socket Init Error\n");
		return false;
	}

	wprintf(L"[Server :: ServerInit]	Complete\n");
	return true;
}

bool CLanServer::ClientShutdown(SESSION *pSession)
{
	int retval = shutdown(pSession->sock, SD_BOTH);
	if (false == retval)
		return false;

	return true;
}

bool CLanServer::ClientRelease(SESSION *pSession)
{
	COMPARE compare;
	compare.iocount = pSession->iocount;
	compare.releaseflag = pSession->bRelease;

	if (false == InterlockedCompareExchange128((LONG64*)_pCompare, (LONG64)0,
		(LONG64)0, (LONG64*)&compare))
	{
		_pCompare->iocount = 0;
		_pCompare->releaseflag = 0;
		return false;
	}

	pSession->bRelease = true;
	closesocket(pSession->sock);

	while (0 < pSession->sendqueue.GetUseCount())
	{
		CPacket *pPacket;
		pSession->sendqueue.Dequeue(&pPacket);
		pPacket->Free();
	}

	while (0 < pSession->packetqueue.GetUseSize())
	{
		CPacket *pPacket;
		pSession->packetqueue.Peek((char*)&pPacket, sizeof(CPacket*));
		pPacket->Free();
		pSession->packetqueue.Dequeue(sizeof(CPacket*));
	}

	pSession->bLoginflag = false;

	ULONG64 sessionkey = pSession->sessionkey;
	unsigned int index = df_GET_INDEX(index, sessionkey);

	InterlockedDecrement(&_connectclient);
	PutIndex(index);

	return true;
}

void CLanServer::PutIndex(unsigned int Index)
{
	_sessionstack.Push(&_pIndex[Index]);
	return;
}

void CLanServer::WorkerThreadUpdate()
{
	int retval;

	while (_bShutdown)
	{
		OVERLAPPED *pOver = NULL;
		SESSION *pSession = NULL;
		DWORD trans = 0;

		retval = GetQueuedCompletionStatus(_hIOCP, &trans, (PULONG_PTR)&pSession,
			(LPWSAOVERLAPPED*)&pOver, INFINITE);

		if (nullptr == pOver)
		{
			if (nullptr == pSession && 0 == trans)
			{
				PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
			}
		}

		if (0 == trans)
			shutdown(pSession->sock, SD_BOTH);
		else if (pOver == &pSession->recvover)
			CompleteRecv(pSession, trans);
		else if (pOver == &pSession->sendover)
			CompleteSend(pSession, trans);

		retval = InterlockedDecrement(&pSession->iocount);
		if (0 >= retval)
		{
			if (0 == retval)
				ClientRelease(pSession);
		}
	}
}

void CLanServer::AcceptThreadUpdate()
{
	int retval = 0;
	SOCKADDR_IN clientaddr;

	while (_bShutdown)
	{
		int addrsize = sizeof(clientaddr);
		SOCKET clientsock = accept(_listensock, (SOCKADDR*)&clientaddr, &addrsize);
		if (INVALID_SOCKET == clientsock)
		{
			wprintf(L"[Server :: Accept]	accept Error\n");
			break;
		}

		InterlockedIncrement(&_accepttps);
		InterlockedIncrement(&_accepttotal);

		OnConnectionRequest((WCHAR*)&clientaddr.sin_addr, clientaddr.sin_port);

		unsigned int *pIndex = GetIndex();
		if (nullptr == pIndex)
		{
			closesocket(clientsock);
			continue;
		}

		if (true == _pSessionarray[*pIndex].bLoginflag)
		{
			ClientRelease(&_pSessionarray[*pIndex]);
			continue;
		}

		unsigned int index = *pIndex;

		_pSessionarray[*pIndex].sessionkey = _sessionkeycnt++;
		df_SET_INDEX(index, _pSessionarray[*pIndex].sessionkey);
		_pSessionarray[*pIndex].sock = clientsock;
		_pSessionarray[*pIndex].recvqueue.Clear();
		_pSessionarray[*pIndex].packetqueue.Clear();
		_pSessionarray[*pIndex].sendflag = true;
		_pSessionarray[*pIndex].sendcount = 0;
		InterlockedIncrement(&_connectclient);
		_pSessionarray[*pIndex].bLoginflag = true;
		_pSessionarray[*pIndex].info.sessionkey = _pSessionarray[*pIndex].sessionkey;
		_pSessionarray[*pIndex].bRelease = false;

		InterlockedIncrement(&_pSessionarray[*pIndex].iocount);
		CreateIoCompletionPort((HANDLE)clientsock, _hIOCP,
			(ULONG_PTR)&_pSessionarray[*pIndex].info,0);
		OnClientJoin(&_pSessionarray[*pIndex].info);
		StartRecvPost(&_pSessionarray[*pIndex]);
	}
}

void CLanServer::MonitorThreadUpdate()
{
	cout << endl;

	struct tm *t = new struct tm;
	time_t timer;

	while (_bShutdown)
	{
		Sleep(1000);
		timer = time(NULL);
		localtime_s(t, &timer);
		if (true == _bMonitorflag)
		{
			wprintf(L"[%d/%d/%d %d:%d:%d]\n", t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			wprintf(L"[ConnectSession		:	%I64d]\n", _connectclient);
			wprintf(L"[Accept_TPS		:	%I64d]\n", _accepttps);
			wprintf(L"[Accept_Total		:	%I64d]\n", _accepttotal);
			wprintf(L"[RecvPacket_TPS		:	%I64d]\n", _recvpackettps);
			wprintf(L"[SendPacket_TPS		:	%I64d]\n", _sendpackettps);
			wprintf(L"[MemoryPool_AllocCount	:	%I64d]\n", CPacket::GetAllocPool());
			wprintf(L"[MemoryPool_UseCount	:	%I64d]\n\n", CPacket::GetUsePool());
		}
		_accepttps = 0;
		_recvpackettps = 0;
		_sendpackettps = 0;
	}
	delete t;
}

void CLanServer::StartRecvPost(SESSION *pSession)
{
	int retval = 0;
	DWORD flags = 0;
	ZeroMemory(&pSession->recvover, sizeof(pSession->recvover));

	WSABUF buf[2];
	DWORD freesize = pSession->recvqueue.GetFreeSize();
	DWORD notbrokenpushsize = pSession->recvqueue.GetNotBrokenPushSize();
	if (0 == freesize && 0 == notbrokenpushsize)
	{
		retval = InterlockedDecrement(&pSession->iocount);
		if (0 >= retval)
		{
			if (0 == retval)
				ClientRelease(pSession);
		}
		else
			shutdown(pSession->sock, SD_BOTH);
		return;
	}
	int bufnum = (notbrokenpushsize < freesize) ? 2 : 1;

	buf[0].buf = pSession->recvqueue.GetWriteBufferPtr();
	buf[0].len = notbrokenpushsize;

	if (2 == bufnum)
	{
		buf[1].buf = pSession->recvqueue.GetBufferPtr();
		buf[1].len = freesize - notbrokenpushsize;
	}

	if (SOCKET_ERROR == WSARecv(pSession->sock, buf, bufnum,
		NULL, &flags, &pSession->recvover, NULL))
	{
		int lasterror = WSAGetLastError();
		if (ERROR_IO_PENDING != lasterror)
		{
			retval = InterlockedDecrement(&pSession->iocount);
			if (0 >= retval)
			{
				if (0 == retval)
					ClientRelease(pSession);
			}
			else
				shutdown(pSession->sock, SD_BOTH);
		}
	}
}

void CLanServer::RecvPost(SESSION *pSession)
{
	InterlockedIncrement(&pSession->iocount);

	int retval = 0;
	DWORD flags = 0;
	ZeroMemory(&pSession->recvover, sizeof(pSession->recvover));

	WSABUF buf[2];
	DWORD freesize = pSession->recvqueue.GetFreeSize();
	DWORD notbrokenpushsize = pSession->recvqueue.GetNotBrokenPushSize();
	if (0 == freesize && 0 == notbrokenpushsize)
	{
		retval = InterlockedDecrement(&pSession->iocount);
		if (0 >= retval)
		{
			if (0 == retval)
				ClientRelease(pSession);
		}
		else
			shutdown(pSession->sock, SD_BOTH);
		return;
	}
	int bufnum = (notbrokenpushsize < freesize) ? 2 : 1;

	buf[0].buf = pSession->recvqueue.GetWriteBufferPtr();
	buf[0].len = notbrokenpushsize;

	if (2 == bufnum)
	{
		buf[1].buf = pSession->recvqueue.GetBufferPtr();
		buf[1].len = freesize - notbrokenpushsize;
	}

	if (SOCKET_ERROR == WSARecv(pSession->sock, buf, bufnum,
		NULL, &flags, &pSession->recvover, NULL))
	{
		int lasterror = WSAGetLastError();
		if (ERROR_IO_PENDING != lasterror)
		{
			retval = InterlockedDecrement(&pSession->iocount);
			if (0 >= retval)
			{
				if (0 == retval)
					ClientRelease(pSession);
			}
			else
				shutdown(pSession->sock, SD_BOTH);
		}
	}	
}

void CLanServer::SendPost(SESSION *pSession)
{
	int retval = 0;
	do
	{
		if (false == InterlockedCompareExchange(&pSession->sendflag, false, true))
			return;

		if (0 == pSession->sendqueue.GetUseCount())
		{
			InterlockedExchange(&pSession->sendflag, true);
			continue;
		}
		ZeroMemory(&pSession->sendover, sizeof(pSession->sendover));

		WSABUF buf[df_MAX_WSABUF_NUM];
		CPacket *pPacket;
		long bufnum = 0;
		int usesize = pSession->sendqueue.GetUseCount();
		if (usesize > df_MAX_WSABUF_NUM)
		{
			bufnum = df_MAX_WSABUF_NUM;
			pSession->sendcount = df_MAX_WSABUF_NUM;
			for (int i = 0; i < df_MAX_WSABUF_NUM; i++)
			{
				pSession->sendqueue.Dequeue(&pPacket);
				pSession->packetqueue.Enqueue((char*)&pPacket, sizeof(CPacket*));
				buf[i].buf = pPacket->GetBufferPtr();
				buf[i].len = pPacket->GetPacketSize();
			}
		}
		else
		{
			bufnum = usesize;
			pSession->sendcount = usesize;
			for (int i = 0; i < usesize; i++)
			{
				pSession->sendqueue.Dequeue(&pPacket);
				pSession->packetqueue.Enqueue((char*)&pPacket, sizeof(CPacket*));
				buf[i].buf = pPacket->GetBufferPtr();
				buf[i].len = pPacket->GetPacketSize();
			}
		}
		InterlockedIncrement(&pSession->iocount);
		if (SOCKET_ERROR == WSASend(pSession->sock, buf, bufnum,
			NULL, 0, &pSession->sendover, NULL))
		{
			int lasterror = WSAGetLastError();
			if (ERROR_IO_PENDING != lasterror)
			{
				retval = InterlockedDecrement(&pSession->iocount);
				if (0 >= retval)
				{
					if (0 == retval)
						ClientRelease(pSession);
				}
				else
					shutdown(pSession->sock, SD_BOTH);
			}
		}
	} while (0 != pSession->sendqueue.GetUseCount());
}

void CLanServer::CompleteRecv(SESSION *pSession, DWORD dwTransfered)
{
	pSession->recvqueue.Enqueue(dwTransfered);
	WORD payloadsize = 0;

	while (df_HEADER_SIZE == pSession->recvqueue.Peek((char*)&payloadsize, df_HEADER_SIZE))
	{
		if (pSession->recvqueue.GetUseSize() < df_HEADER_SIZE + payloadsize)
			break;
		pSession->recvqueue.Dequeue(df_HEADER_SIZE);
		CPacket *pPacket = CPacket::Alloc();
		if (pPacket->GetFreeSize() < payloadsize)
		{
			shutdown(pSession->sock, SD_BOTH);
			return;
		}
		pSession->recvqueue.Dequeue(pPacket->GetWritePtr(), payloadsize);
		pPacket->PushData(payloadsize);

		if (false == OnRecv(pSession, pPacket))
			return;

		pPacket->Free();
	}
	RecvPost(pSession);
}

void CLanServer::CompleteSend(SESSION *pSession, DWORD dwTransfered)
{
	CPacket *pPacket[df_MAX_WSABUF_NUM];
	pSession->packetqueue.Peek((char*)&pPacket, sizeof(CPacket*));
	for (int i = 0; i < pSession->sendcount; i++)
	{
		pPacket[i]->Free();
		pSession->packetqueue.Dequeue(sizeof(CPacket*));
	}
	InterlockedExchange(&pSession->sendflag, true);

	SendPost(pSession);
}

bool CLanServer::OnRecv(SESSION *pSession, CPacket *pPacket)
{
	_recvpackettps++;

	LONG64 data = 0;
	*pPacket >> data;

	CPacket *pPacket = CPacket::Alloc();
	*pPacket << data;

	bool retval = false;
	if (pSession->bRelease == false)
		retval = SendPacket(pSession->sessionkey, pPacket);
	pPacket->Free();

	return retval;
}

unsigned int* CLanServer::GetIndex()
{
	unsigned int *index = nullptr;
	_sessionstack.Pop(&index);
	return index;
}