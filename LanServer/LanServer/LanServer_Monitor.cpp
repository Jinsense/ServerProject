#include "LanServer_Monitor.h"

CLanServerMonitor::CLanServerMonitor()
{

}

CLanServerMonitor::~CLanServerMonitor()
{

}

void CLanServerMonitor::OnClientJoin(INFO *pInfo)
{
	CPacket *pPacket = CPacket::Alloc();
	__int64 data = 0x7fffffffffffffff;
	*pPacket << data;

	bool retval = SendPacket(pInfo->sessionkey, pPacket);
	pPacket->Free();
	return;
}

void CLanServerMonitor::OnClientLeave(ULONG64 sessionkey)
{

}

void CLanServerMonitor::OnConnectionRequest(WCHAR *pClientIP, int port)
{

}

void CLanServerMonitor::OnError(int errorcode, WCHAR *pError)
{

}
