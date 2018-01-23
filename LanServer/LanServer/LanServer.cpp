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

}

long CLanServer::GetClientCount()
{

}

bool CLanServer::ServerStart(WCHAR *pOpenIP, int port, int maxworkerthread,
	bool bNodelay, int maxsession)
{
	wprintf(L"[Server :: ServerStart] Start\n");

	int retval = 0;
	setlocale(LC_ALL, "Korean");
	InitializeCriticalSection(&_sessioncs);

	CPacket::MemoryPool_Init();

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
	return true;
}

bool CLanServer::SendPacket(ULONG64 sessionkey, CPacket *pPacket)
{
	return true;
}

SESSION* CLanServer::SessionAcquireLock(ULONG64 sessionkey)
{

}

void CLanServer::SessionAcquireFree(SESSION *pSession)
{

}

bool CLanServer::ServerInit()
{
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