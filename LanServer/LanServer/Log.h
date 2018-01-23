#ifndef _LANSERVER_LOG_LOG_H_
#define _LANSERVER_LOG_LOG_H_

#include <direct.h>
#include <wchar.h>
#include <windows.h>
#include <iostream>
#include <strsafe.h>
#include <string>
#include <winerror.h>

enum en_LOG_LEVEL
{
	LOG_DEBUG = 0,
	LOG_WARNING,
	LOG_ERROR,
	LOG_SYSTEM,
};

class CSystemLog
{
private:

	CSystemLog(en_LOG_LEVEL LogLevel);
	~CSystemLog();


public:

	//------------------------------------------------------
	// �̱��� Ŭ����, 
	//------------------------------------------------------
	static CSystemLog *GetInstance(en_LOG_LEVEL LogLevel = LOG_DEBUG)
	{
		static CSystemLog Log(LogLevel);
		return &Log;
	}

	//------------------------------------------------------
	// �ܺο��� �α׷��� ����
	//------------------------------------------------------
	void SetLogLevel(en_LOG_LEVEL LogLevel) { _SaveLogLevel = LogLevel; }

	//------------------------------------------------------
	// �α� ��� ����.
	//------------------------------------------------------
	void SetLogDirectory(WCHAR *szDirectory)
	{
		_wmkdir(szDirectory);
		wsprintf(_SaveDirectory, L"%s\\", szDirectory);
	}

	//------------------------------------------------------
	// ���� �α� ����� �Լ�.
	//------------------------------------------------------
	void Log(WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szStringFormat, ...);

	//------------------------------------------------------
	// BYTE ���̳ʸ��� ���� �α� ���
	//------------------------------------------------------
	void LogHex(WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szLog, BYTE *pByte, int iByteLen);

	//------------------------------------------------------
	// SessionKey 64�� ��� ����. �̴� ���ڿ��� �ƴ϶� �������� ���� ����.
	//------------------------------------------------------
	void LogSessionKey(WCHAR *szType, en_LOG_LEVEL LogLevel, WCHAR *szLog, BYTE *pSessionKey);

private:

	unsigned long	_LogNo;
	SRWLOCK			_srwLock;

	en_LOG_LEVEL	_SaveLogLevel;
	WCHAR			_SaveDirectory[25];
};

#endif _LANSERVER_LOG_LOG_H_