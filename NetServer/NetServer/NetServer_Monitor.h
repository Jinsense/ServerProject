#ifndef _NETSERVER_IOCP_MONITOR_H_
#define _NETSERVER_IOCP_MONITOR_H_

#include "NetServer.h"

class CNetServerMonitor : public CNetServer
{
public:
	CNetServerMonitor();
	~CNetServerMonitor();
	virtual void		OnClientJoin(st_SessionInfo *pInfo);
	virtual void		OnClientLeave(unsigned __int64 iSessionKey);
	virtual void		OnConnectionRequest(WCHAR *pClientIP, int iPort);
	virtual void		OnError(int iErrorCode, WCHAR *pError);
};

#endif _NETSERVER_IOCP_MONITOR_H_