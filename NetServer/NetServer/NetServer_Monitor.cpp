#include "NetServer_Monitor.h"

CNetServerMonitor::CNetServerMonitor()
{

}

CNetServerMonitor::~CNetServerMonitor()
{

}

void CNetServerMonitor::OnClientJoin(st_SessionInfo *pInfo)
{
	CPacket *_pPacket = CPacket::Alloc();
	__int64 _iLoginData = 0x7fffffffffffffff;
	*_pPacket << _iLoginData;

	bool _bRetval = SendPacket(pInfo->iSessionKey, _pPacket);
	_pPacket->Free();
}

void CNetServerMonitor::OnClientLeave(unsigned __int64 iSessionKey)
{

}

void CNetServerMonitor::OnConnectionRequest(WCHAR *pClientIP, int iPort)
{

}

void CNetServerMonitor::OnError(int iErrorCode, WCHAR *pError)
{
	
}