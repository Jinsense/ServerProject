#ifndef _LANSERVER_IOCP_LANSERVERMONITOR_H_
#define _LANSERVER_IOCP_LANSERVERMONITOR_H_

#include "LanServer.h"

class CLanServerMonitor : public CLanServer
{
	CLanServerMonitor();
	~CLanServerMonitor();

	virtual void	OnClientJoin(SESSION *pInfo);
	virtual void	OnClientLeave(ULONG64 sessionkey);
	virtual void	OnConnectionRequest(WCHAR *pClientIP, int port);
	virtual void	OnError(int errorcode, WCHAR *pError);
};

#endif _LANSERVER_IOCP_LANSERVERMONITOR_H_
