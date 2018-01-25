#include <conio.h>

#include "LanServer_Monitor.h"

int main()
{
	int retval = 0;
	int in = 0;

	CLanServerMonitor server;
	const WCHAR *pOpenIP = L"0.0.0.0";
	SYSTEM_INFO sysinfo;

	GetSystemInfo(&sysinfo);

	if (false == (retval = server.ServerStart(pOpenIP, 6000,
		sysinfo.dwNumberOfProcessors * 2, true, df_MAX_CLINET_NUM)))
	{
		wprintf(L"[Server :: ServerStart] Error\n");
		return 0;
	}

	while (1)
	{
		in = _getch();
		switch (in)
		{
		case 'q': case 'Q':
		{
			wprintf(L"[Main] 서버를 종료합니다.\n");
			_getch();
		}
		break;
		case 'm': case'M':
		{
			if (false == server.GetMonitorMode())
			{
				server.SetMonitorMode(true);
				wprintf(L"[Main] MonitorMode Start\n");
			}
			else
			{
				server.SetMonitorMode(false);
				wprintf(L"[Main] MonitorMode Stop\n");
			}
		}
		break;
		default:
			break;
		}
	}
	return 0;
}