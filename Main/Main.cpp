// Main.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "IOCP.h"
#include "FtpSvrIoContext.h"

int _tmain(int argc, _TCHAR* argv[])
{
	char* pContext = NULL;
	static char szUser[MAX_NAME_LEN] = "fuck gongfei.\r\n";
	strtok_s(szUser, " ", &pContext);

	CIOCP Iocp;
	if (!Iocp.Start())
	{
		HL_PRINT(_T("������ɶ˿�ʧ�ܣ�\r\n"));
		return 0;
	}

	CFtpSvrIoContext FtpSvr;
	if (FtpSvr.StartSvr(5555))
	{
		HL_PRINT(_T("FTP Server Running...\r\n"));
	}
	else
	{
		HL_PRINT(_T("FTP Server Start Failed.\r\n"));
	}

	Sleep(INFINITE);
	return 0;
}

