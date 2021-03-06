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

	int SendResponse(const char* szFormat, ...);

protected:
	int ParseCommand(CIoBuffer* pIoBuff);

	char* RelativeDirectory(char* szDir);

	char* Back2Slash(char* szPath);

	int CheckDirectory(string szDir, int opt, string& szResult);

	void DoChangeDirectory(string szDir);

	BOOL DataConn(DWORD dwIP, WORD wPort, int nMode);

	char* GetLocalAddress();
	char* ConvertCommandAddress(char* szAddress, WORD wPort);

	BOOL ConvertDotAddress(const string& strText);

	UINT FileListToString(string& szBuff, BOOL bDetails);
	int GetFileList(LPFILE_INFO pFI, UINT nArraySize, const char* szPath);

	SOCKET DataAccept();
	int DataSend(SOCKET s, char* buff, int nBufSize);
	int DataRecv(SOCKET s, const char* szFilename);

	DWORD WriteToFile(SOCKET s, const char* szFilename);

	char* AbsoluteDirectory(string& szDir);

	int CheckFileName(string szFilename, int nOption, string& szResult);
	BOOL GetLocalPath(string szRelativePath, CString& szLocalPath);

private:
	BOOL	m_bLoggedIn;
	BOOL	m_bPassive = FALSE;
	int		m_nStatus;
	string m_szCurrDir;

	DWORD  m_dwRemoteAddr;
	int	   m_nRemotePort;

	// DATA FTP �׽���
	SOCKET m_sDataIo;
};

