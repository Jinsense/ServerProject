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

	return true;
}

bool CLanServer::ClientRelease(SESSION *pSession)
{
	return true;
}

void CLanServer::PutIndex(unsigned int Index)
{

}

void CLanServer::WorkerThreadUpdate()
{

}

void CLanServer::AcceptThreadUpdate()
{

}

void CLanServer::MonitorThreadUpdate()
{

}

void CLanServer::StartRecvPost(SESSION *pSession)
{

}

void CLanServer::RecvPost(SESSION *pSession)
{

}

void CLanServer::SendPost(SESSION *pSession)
{

}

void CLanServer::CompleteRecv(SESSION *pSession, DWORD dwTransfered)
{

}

void CLanServer::CompleteSend(SESSION *pSession, DWORD dwTransfered)
{

}

bool CLanServer::OnRecv(SESSION *pSession, CPacket *pPacket)
{
	return true;
}

unsigned int* CLanServer::GetIndex()
{

}