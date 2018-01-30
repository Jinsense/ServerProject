#include "LanServer_Monitor.h"

CLanServerMonitor::CLanServerMonitor()
{

}

CLanServerMonitor::~CLanServerMonitor()
{

}

void CLanServerMonitor::OnClientJoin(st_SessionInfo *pInfo)
{
	CPacket *_pPacket = CPacket::Alloc();
	__int64 _iLoginData = 0x7fffffffffffffff;
	*_pPacket << _iLoginData;

	bool _bRetval = SendPacket(pInfo->iSessionKey, _pPacket);
	_pPacket->Free();
}

void CLanServerMonitor::OnClientLeave(unsigned __int64 iSessionKey)
{

}

void CLanServerMonitor::OnConnectionRequest(WCHAR *pClientIP, int iPort)
{

}

void CLanServerMonitor::OnError(int iErrorCode, WCHAR *pError)
{
	
}