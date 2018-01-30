#ifndef _LANSERVER_IOCP_MONITOR_H_
#define _LANSERVER_IOCP_MONITOR_H_

#include "LanServer.h"

class CLanServerMonitor : public CLanServer
{
public:
	CLanServerMonitor();
	~CLanServerMonitor();
	virtual void		OnClientJoin(st_SessionInfo *pInfo);
	virtual void		OnClientLeave(unsigned __int64 iSessionKey);
	virtual void		OnConnectionRequest(WCHAR *pClientIP, int iPort);
	virtual void		OnError(int iErrorCode, WCHAR *pError);
};

#endif _LANSERVER_IOCP_MONITOR_H_