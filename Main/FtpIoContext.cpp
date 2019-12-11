#include "stdafx.h"
#include "FtpIoContext.h"


CFtpIoContext::CFtpIoContext(SOCKET sock) :CIoContext(sock)
{
}


CFtpIoContext::~CFtpIoContext()
{
}

void CFtpIoContext::ProcessJob(CIoBuffer* pIoBuffer)
{
	FtpProcessRequest(pIoBuffer);
}

void CFtpIoContext::FtpProcessRequest(CIoBuffer* pIoBuff)
{
	if (pIoBuff->m_vtBuffer[pIoBuff->m_nIoSize - 2] == '\r'      // Ҫ��֤�����\r\n
		&& pIoBuff->m_vtBuffer[pIoBuff->m_nIoSize - 1] == '\n'
		&& pIoBuff->m_nIoSize > 2)
	{
		if (!pIoBuff->m_bLoggedIn)
		{
			if (LogonSvr(pIoBuff) == LOGGED_IN)
				pIoBuff->m_bLoggedIn = true;
		}
		else
		{

		}
	}
}

bool CFtpIoContext::Welcome()
{
	char* szWelcome = "220 ��ӭ��¼��FTP Server  -- By:Langer   \r\n";
	CIoBuffer* pBuffer = new CIoBuffer();
	pBuffer->AddData(szWelcome, strlen(szWelcome));

	return AsyncSend(pBuffer);
}

int CFtpIoContext::LogonSvr(CIoBuffer* pIoBuff)
{
	const char* szUserOK = "331 User name okay, need password.\r\n";
	const char* szLoggedIn = "230 User logged in, proceed.\r\n";

	int nRet = 0;
	char* pContext = NULL;
	static char szUser[MAX_NAME_LEN] = {}, szPwd[MAX_PWD_LEN] = {};
	string szCmd;
	strtok_s(pIoBuff->m_vtBuffer, " ", &pContext);
	szCmd = pIoBuff->m_vtBuffer;
	transform(szCmd.begin(), szCmd.end(), szCmd.begin(), ::toupper);

	if (szCmd != "USER" && szCmd != "PASS")
	{
		SendResponse("530 Please login with USER and PASS.\r\n");
		return USER_OK;;
	}

	if (szCmd == "USER")
	{
		//ȡ�õ�¼�û���		
		sprintf_s(szUser, "%s", pContext);
		strtok_s(szUser, "\r\n", &pContext);
		//��Ӧ��Ϣ
		char szResponse[MAX_PATH] = {};
		sprintf_s(szResponse, "331 Password required for %s\r\n", szUser);
		SendResponse(szResponse);
		return USER_OK;
	}

	if (szCmd == "PASS")
	{
		sprintf_s(szPwd, "%s", pContext);
		strtok_s(szPwd, "\r\n", &pContext);
		//��֤�û����Ϳ���
		if (_stricmp(szPwd, DEFAULT_PASS) || _stricmp(szUser, DEFAULT_USER))
		{
			SendResponse("530 Not logged in, user or password incorrect!\r\n");
			nRet = LOGIN_FAILED;
		}
		else
		{
			SendResponse("230 User successfully logged in.\r\n");
			nRet = LOGGED_IN;
		}
	}

	return nRet;
}

int CFtpIoContext::SendResponse(const char* szResponse)
{
	CIoBuffer* pBuffer = new CIoBuffer();
	pBuffer->AddData(szResponse, strlen(szResponse));
	return AsyncSend(pBuffer);
}
