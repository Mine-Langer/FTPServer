#pragma once
#include "IoContext.h"

class CFtpIoContext :public CIoContext
{
public:
	CFtpIoContext(SOCKET sock);
	virtual ~CFtpIoContext();

	virtual void ProcessJob(CIoBuffer* pIoBuffer);

	void FtpProcessRequest(CIoBuffer* pIoBuffer);
	
	bool Welcome();

	int LogonSvr(CIoBuffer* pIoBuffer);

	int SendResponse(const char* szResponse);

protected:
	int ParseCommand(CIoBuffer* pIoBuff);

	char* RelativeDirectory(char* szDir);

	char* Back2Slash(char* szPath);

	int CheckDirectory(string szDir, int opt, string& szResult);

	void DoChangeDirectory(string szDir);

	int DataConn(DWORD dwIP, WORD wPort, int nMode);

	char* GetLocalAddress();
	char* ConvertCommandAddress(char* szAddress, WORD wPort);

private:
	BOOL	m_bLoggedIn;
	int		m_nStatus;
	string m_szCurrDir;
};

