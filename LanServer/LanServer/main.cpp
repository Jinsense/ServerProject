#include "LanServer_Monitor.h"

#include <conio.h>

int main()
{
	int _Retval;
	int _In;

	CLanServerMonitor _Server;
	SYSTEM_INFO _SysInfo;
	const WCHAR *pIP = L"0.0.0.0";

	GetSystemInfo(&_SysInfo);
	
	if ((_Retval = _Server.ServerStart(pIP, 6000, 
				_SysInfo.dwNumberOfProcessors * 2, true, 
				MAX_CLIENT_NUMBER)) == false)
	{
		wprintf(L"[Server :: Server_Start] Error\n");
		return 0;
	}

	while (!_Server.GetShutDownMode())
	{
		_In = _getch();
		switch (_In)
		{
		case 'q': case 'Q':
		{
			_Server.SetShutDownMode(true);
			wprintf(L"[Main] ������ �����մϴ�.\n");
			_getch();
			break;
		}
		case 'm': case 'M':
		{
			if (false == _Server.GetMonitorMode())
			{
				_Server.SetMonitorMode(true);
				wprintf(L"[Main] MonitorMode Start\n");
			}
			else
			{
				_Server.SetMonitorMode(false);
				wprintf(L"[Main] MonitorMode Stop\n");
			}
		}
		}
	}
	return 0;
}