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

	BOOL GetDirectoryList(string szDirectory, string &szResult);

	char* GetLocalAddress();
	char* ConvertCommandAddress(char* szAddress, WORD wPort);

	BOOL ConvertDotAddress(const string& strText);

	UINT FileListToString(string& szBuff, BOOL bDetails);
	int GetFileList(LPFILE_INFO pFI, UINT nArraySize, const char* szPath);

	int DataSend(SOCKET s, char* buff, int nBufSize);

	char* AbsoluteDirectory(string& szDir);

private:
	BOOL	m_bLoggedIn;
	BOOL	m_bPassive = FALSE;
	int		m_nStatus;
	string m_szCurrDir;

	DWORD  m_dwRemoteAddr;
	int	   m_nRemotePort;

	SOCKET m_sDataIo;
};

