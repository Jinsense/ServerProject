#include "Log.h"

CSystemLog::CSystemLog(en_LOG_LEVEL LogLevel)
{
	_SaveLogLevel = LogLevel;
	InitializeSRWLock(&_srwLock);
	_LogNo = 0;

}

CSystemLog::~CSystemLog()
{

}

void CSystemLog::Log(WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szStringFormat, ...)
{

	if (_SaveLogLevel <= LogLevel)
	{
		HRESULT hResult;
		WCHAR szFileName[256];
		WCHAR szInError[256];
		WCHAR szInMessage[512];
		WCHAR szBuf[1024];
		InterlockedIncrement(&_LogNo);

		AcquireSRWLockExclusive(&_srwLock);

		SYSTEMTIME st;
		GetLocalTime(&st);


		hResult = StringCchPrintf(szFileName, 256, L"%d%02d %sLog.txt", st.wYear, st.wMonth, szType);

		if (FAILED(hResult))
		{
			if (hResult == STRSAFE_E_INVALID_PARAMETER)
			{
				//	���̰� �� �αװ� ©���� ���
				//	�αװ� ©�ȴٴ� �α׸� �߰��� �����
				//	� �αװ� ©�ȴ����� ������, �Ȱ��� �����ϸ� �αװ� ©���Ƿ�
				//	���̸� �߶� �־���
			}
		}

		va_list va;
		va_start(va, szStringFormat);
		hResult = StringCchVPrintf(szInMessage, 512, szStringFormat, va);
		va_end(va);

		if (FAILED(hResult))
		{

		}

		hResult = StringCchPrintf(szBuf, 1024, L"[%02d/%02d/%d %02d:%02d:%02d:%08d] %s\n",
			st.wMonth, st.wDay, st.wYear,
			st.wHour, st.wMinute, st.wSecond, _LogNo, szInMessage);

		if (FAILED(hResult))
		{

		}

		//	������ ���� �ݴ� ���� ���� ������ �ּҷ� ������.
		FILE * pFile;
		_wfopen_s(&pFile, szFileName, L"a, ccs=UTF-16LE");
		fwrite(szBuf, sizeof(WCHAR), wcslen(szBuf), pFile);
		fclose(pFile);
		ReleaseSRWLockExclusive(&_srwLock);
	}
}

void CSystemLog::LogHex(WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szLog, BYTE *pByte, int iByteLen)
{

}

void CSystemLog::LogSessionKey(WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szLog, BYTE *pSessionKey)
{


}