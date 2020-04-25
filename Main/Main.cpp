// Main.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "IOCP.h"
#include "FtpSvrIoContext.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CIOCP Iocp;
	if (!Iocp.Start())
	{
		HL_PRINT(_T("创建完成端口失败！\r\n"));
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

