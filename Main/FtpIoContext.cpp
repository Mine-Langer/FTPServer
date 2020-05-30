#include "stdafx.h"
#include "FtpIoContext.h"

const char* MONTHNAME[] = { "", "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

CFtpIoContext::CFtpIoContext(SOCKET sock) :CIoContext(sock), m_bLoggedIn(FALSE)
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
	if (pIoBuff->m_vtBuffer[pIoBuff->m_nIoSize - 2] == '\r'      // 要保证最后是\r\n
		&& pIoBuff->m_vtBuffer[pIoBuff->m_nIoSize - 1] == '\n'
		&& pIoBuff->m_nIoSize > 2)
	{
		HL_PRINTA("[RECV] %s", pIoBuff->m_vtBuffer);
		if (!m_bLoggedIn)
		{
			if (LogonSvr(pIoBuff) == LOGGED_IN)
				m_bLoggedIn = TRUE;
		}
		else
		{
			ParseCommand(pIoBuff);
		}
	}
}

bool CFtpIoContext::Welcome()
{
	char* szWelcome = "220 welcome to FTP Server  -- By:Langer   \r\n";
	CIoBuffer* pBuffer = new CIoBuffer();
	pBuffer->AddData(szWelcome, strlen(szWelcome));

	char szText[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, szText);
	m_szCurrDir = szText;
	SetCurrentDirectory(szText);

	return AsyncSend(pBuffer);
}

int CFtpIoContext::LogonSvr(CIoBuffer* pIoBuff)
{
	const char* szUserOK = "331 User name okay, need password.\r\n";
	const char* szLoggedIn = "230 User logged in, proceed.\r\n";

	int nRet = 0;
	char* pContext = NULL;
	static char szUser[MAX_NAME_LEN] = {}, szPwd[MAX_PWD_LEN] = {};
	string szCmd, szArg;
	strtok_s(pIoBuff->m_vtBuffer, " ", &pContext);
	szCmd = pIoBuff->m_vtBuffer;
	szArg = pContext;

	transform(szCmd.begin(), szCmd.end(), szCmd.begin(), ::toupper);

	if (szCmd != "USER" && szCmd != "PASS")
	{
		SendResponse("530 Please login with USER and PASS.\r\n");
		return USER_OK;
	}

	if (szCmd == "USER")
	{
		//取得登录用户名		
		sprintf_s(szUser, "%s", szArg.c_str());
		strtok_s(szUser, "\r\n", &pContext);
		//响应信息
		m_nStatus = STATUS_LOGIN;
		char szResponse[MAX_PATH] = {};
		sprintf_s(szResponse, "331 Password required for %s\r\n", szUser);
		SendResponse(szResponse);
		return USER_OK;
	}

	if (szCmd == "PASS")
	{
		sprintf_s(szPwd, "%s", szArg.c_str());
		strtok_s(szPwd, "\r\n", &pContext);
		//验证用户名和口令
		if (_stricmp(szPwd, DEFAULT_PASS) || _stricmp(szUser, DEFAULT_USER))
		{
			SendResponse("530 Not logged in, user or password incorrect!\r\n");
			nRet = LOGIN_FAILED;
		}
		else
		{
			m_szCurrDir = "/";
			m_nStatus = STATUS_IDLE;
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
	HL_PRINTA("    %s", szResponse);
	return AsyncSend(pBuffer);
}

int CFtpIoContext::ParseCommand(CIoBuffer* pIoBuff)
{
	//char szCmd[MAX_REQ_LEN] = {};
	char szCurrDir[MAX_PATH] = {};

	char* pContext = NULL;
	string szCmd, szArg;
	strtok_s(pIoBuff->m_vtBuffer, "\r\n", &pContext);
	strtok_s(pIoBuff->m_vtBuffer, " ", &pContext);
	
	szCmd = pIoBuff->m_vtBuffer;
	szArg = pContext;

	if (szCmd.empty())
		return -1;

	transform(szCmd.begin(), szCmd.end(), szCmd.begin(), ::toupper);

	if (szCmd == "PORT")
	{
		vector<string> vecData;
		ConvertDotAddress(szArg);
		
		m_bPassive = FALSE;
		SendResponse("200 Port command successful.\r\n");
		
		return CMD_OK;
	}
	else if (szCmd == "PASV")
	{
		//在PASV模式下，服务端创建新的监听套接字来连接客户端
		if (-1 == DataConn(htonl(INADDR_ANY), PORT_BIND, MODE_PASV))
			return -1;

		char* szCommandAddress = ConvertCommandAddress(/*GetLocalAddress()*/"127.0.0.1", PORT_BIND);

		char szText[MAX_PATH] = {};
		sprintf_s(szText, "227 Entering Passive Mode (%s).\r\n", szCommandAddress);
		if (!SendResponse(szText))
			return -1;

		m_bPassive = TRUE;
		return PASSIVE_MODE;
	}
	else if (szCmd == "NLST" || szCmd == "LIST")
	{
		SOCKET sAccept;
		if (m_bPassive)
		{
			sAccept = DataAccept(m_sDataIo);
		}

		const char* szOpeningAMode = "150 Opening ASCII mode data connection for ";

		char szText[1024] = { 0 };
		if (!m_bPassive)
			sprintf_s(szText, 1024, "%s/bin/ls.\r\n", szOpeningAMode);
		else
			sprintf_s(szText, 1024, "125 Data connection already open; Transfer starting.\r\n");
		
		if (!SendResponse(szText))
			return -1;
		
		// 获取文件列表信息
		string szList;
		UINT nStrLen = FileListToString(szList, TRUE);
		/*if (!GetDirectoryList(szArg, szList))
			return -1;*/

		if (!m_bPassive)
		{
			if (!DataConn(m_dwRemoteAddr, m_nRemotePort, MODE_PORT))
				return -1;

			if (-1 == DataSend(m_sDataIo, const_cast<char*>(szList.c_str()), szList.size()))
				return -1;
			closesocket(m_sDataIo);
		}
		else
		{
			DataSend(sAccept, const_cast<char*>(szList.c_str()), szList.size());
			closesocket(sAccept);
		}

		if (!SendResponse("226 Transfer complete.\r\n"))
			return -1;

		return TRANS_COMPLETE;
	}
	else if (szCmd == "RETR")
	{

	}
	else if (szCmd == "STOR")
	{

	}
	else if (szCmd == "QUIT")
	{
		if (!SendResponse("221 Good bye,欢迎下次再来.\r\n"))
			return -1;
		return FTP_QUIT;
	}
	else if (szCmd == "XPWD" || szCmd == "PWD")
	{
		GetCurrentDirectoryA(MAX_PATH, szCurrDir);
		char szText[MAX_PATH] = {};
		sprintf_s(szText, "257 \"%s\" is current directory.\r\n", m_szCurrDir.c_str());
		if (!SendResponse(szText))
			return -1;
		return CURR_DIR;
	}
	else if (szCmd == "CWD" || szCmd == "CDUP")
	{
		if (szArg.empty())
			szArg = "\\";

		char szDir[MAX_PATH] = { 0 };
		if (szCmd == "CDUP")
			strcpy_s(szDir, MAX_PATH, "..");
		else
			strcpy_s(szDir, AbsoluteDirectory(szArg));

		if (!SetCurrentDirectory(szDir))
		{
			sprintf_s(szCurrDir, MAX_PATH, "\\%s", szDir);
			sprintf_s(szDir, MAX_PATH, "550 %s No such file or directory.\r\n", RelativeDirectory(szCurrDir));
			SendResponse(szDir);
			return CANNOT_FIND;
		}
		else
		{
			GetCurrentDirectoryA(MAX_PATH, szCurrDir);
			sprintf_s(szDir, MAX_PATH, "250 Directory changed to /%s.\r\n", RelativeDirectory(szCurrDir));
			SendResponse(szDir);
			return DIR_CHANGED;
		}
	}
	else if (szCmd == "SYST")
	{
		if (!SendResponse("215 Windows 7.0\r\n"))
			return -1;
		return OS_TYPE;
	}
	else if (szCmd == "TYPE")
	{
		char szText[MAX_PATH] = {};
		sprintf_s(szText, "200 Type set to %s \r\n", szArg.c_str());
		if (!SendResponse(szText))
			return -1;
		return CMD_OK;
	}
	else if (szCmd == "REST")
	{

		SendResponse("350 Restarting at \r\n");
	}
	else if (szCmd == "NOOP")
	{
		char szText[MAX_PATH] = {};
		sprintf_s(szText, MAX_PATH, "200 NOOP command successful.\r\n");
		if (!SendResponse(szText))
			return -1;
		return CMD_OK;
	}
	else
	{
		//其余都是无效命令
		char szText[MAX_PATH] = {};
		sprintf_s(szText, "500 '%s' command not understand.\r\n", szCmd.c_str());
		if (!SendResponse(szText))
			return -1;

	}
	return 0;
}

char* CFtpIoContext::RelativeDirectory(char* szDir)
{
	return Back2Slash(szDir);
}

char* CFtpIoContext::Back2Slash(char* szPath)
{
	int idx = 0;
	if (NULL == szPath) return NULL;
	_strlwr_s(szPath, MAX_PATH);
	while (szPath[idx])
	{
		if (szPath[idx] == '\\')
		{
			szPath[idx] = '/';
		}
		idx++;
	}
	return szPath;
}

void CFtpIoContext::DoChangeDirectory(string szDir)
{
	string szLocalPath;
	int nRet = CheckDirectory(szDir, FTP_LIST, szLocalPath);
	switch (nRet)
	{
	case ERROR_ACCESS_DENIED:
		break;
	case ERROR_PATH_NOT_FOUND:
		break;
	default:
		break;
	}
}

int CFtpIoContext::DataConn(DWORD dwIP, WORD wPort, int nMode)
{
	m_sDataIo = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_sDataIo == INVALID_SOCKET)
		return -1;

	SOCKADDR_IN inetAddr = {};
	inetAddr.sin_family = AF_INET;
	if (MODE_PASV == nMode) 
	{
		inetAddr.sin_port = htons(wPort);
		inetAddr.sin_addr.s_addr = dwIP;
	}
	else 
	{
		inetAddr.sin_port = htons(DATA_FTP_PORT);
		inetAddr.sin_addr.s_addr = inet_addr(GetLocalAddress());
	}

	BOOL optVal = TRUE;
	if (setsockopt(m_sDataIo, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, sizeof(optVal)) == SOCKET_ERROR) {
		HL_PRINT(_T("setsockopt失败，错误码：%d\r\n"), WSAGetLastError());
		closesocket(m_sDataIo);
		return -1;
	}

	if (bind(m_sDataIo, (SOCKADDR*)&inetAddr, sizeof(inetAddr)) == SOCKET_ERROR) {
		HL_PRINT(_T("bind套接字失败, 错误码:%d\r\n"), WSAGetLastError());
		closesocket(m_sDataIo);
		return -1;
	}

	if (MODE_PASV == nMode) {
		if (listen(m_sDataIo, SOMAXCONN) == SOCKET_ERROR) {
			HL_PRINT(_T("listen套接字失败,错误码:%d\r\n"), WSAGetLastError());
			closesocket(m_sDataIo);
			return -1;
		}
	}
	else if (MODE_PORT == nMode)
	{
		SOCKADDR_IN addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(wPort);
		addr.sin_addr.s_addr = dwIP;
		if (connect(m_sDataIo, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
			HL_PRINT(_T("connect套接字失败,错误码:%d\r\n"), WSAGetLastError());
			closesocket(m_sDataIo);
			return -1;
		}
	}
	return 0;
}

BOOL CFtpIoContext::GetDirectoryList(string szDirectory, string &szResult)
{
	string strDirectory = szDirectory;
	if (strDirectory.empty())
		strDirectory = m_szCurrDir;

	string strLocalPath;
	int nResult = CheckDirectory(strDirectory, FTP_LIST, strLocalPath);
	switch (nResult)
	{
	case ERROR_ACCESS_DENIED:
		SendResponse("550 \"%s\": Permission denied.\r\n"/*, szDirectory.c_str()*/);
		return FALSE;

	case ERROR_PATH_NOT_FOUND:
		SendResponse("550 \"%s\": Directory not found.\r\n"/*, szDirectory.c_str()*/);
		return FALSE;

	default:
		break;
	}

	//检测文件是否是目录
	/*	BOOL bFound = FALSE;
	if ((GetFileAttributesA(strLocalPath.c_str())&FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) 
	{
		string strPath = strLocalPath;
		if (strPath.at(strPath.size() - 2) == '\\')
			bFound = TRUE;
	}*/

	WIN32_FIND_DATAA wfd = {};
	HANDLE hFind = FindFirstFileA(strLocalPath.c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	do 
	{
		//文件权限
		if (wfd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			szResult += "drwx------";
		else
			szResult += "-rwx------";

		//文件组
		szResult += " 1 user group ";

		//文件大小
		string szLength = to_string(wfd.nFileSizeHigh);
		string szFiller = "            ";
		szResult += szFiller.substr(0, szFiller.length() - szLength.length());
		szResult += szLength;

		//文件日期
		SYSTEMTIME sysTime = { 0 };
		FileTimeToSystemTime(&wfd.ftLastWriteTime, &sysTime);
		szResult = szResult + to_string(sysTime.wYear) + "-" + to_string(sysTime.wMonth) + "-" + to_string(sysTime.wDay)
			+ " " + to_string(sysTime.wHour) + ":" + to_string(sysTime.wMinute) + ":" + to_string(sysTime.wSecond);

		//文件名称
		szResult += wfd.cFileName;
		szResult += "\r\n";

	} while (FindNextFileA(hFind, &wfd));

	FindClose(hFind);
	return TRUE;
}

char* CFtpIoContext::GetLocalAddress()
{
	IN_ADDR *pInaddr = NULL;
	LPHOSTENT lpHostent;
	char szLocalAddr[80] = { 0 };

	int nRet = gethostname(szLocalAddr, sizeof(szLocalAddr));
	if (nRet == SOCKET_ERROR)
		return NULL;

	lpHostent = gethostbyname(szLocalAddr);
	if (lpHostent == NULL)
		return NULL;

	pInaddr = reinterpret_cast<LPIN_ADDR>(lpHostent->h_addr);
	int nLen = strlen(inet_ntoa(*pInaddr));
	if (nLen > sizeof(szLocalAddr)) {
		WSASetLastError(WSAEINVAL);
		return NULL;
	}

	return inet_ntoa(*pInaddr);
}

char* CFtpIoContext::ConvertCommandAddress(char* szAddress, WORD wPort)
{
	char szPort[10] = { 0 }, szIpAddress[20] = { 0 };
	static char szResult[30] = { 0 };
	
	sprintf_s(szPort, "%d,%d", wPort / 256, wPort % 256);
	sprintf_s(szIpAddress, "%s,", szAddress);
	int idx = 0;
	while (szIpAddress[idx])
	{
		if (szIpAddress[idx] == '.')
			szIpAddress[idx] = ',';
		idx++;
	}
	sprintf_s(szResult, 30, "%s%s", szIpAddress, szPort);
	return szResult;
}

BOOL CFtpIoContext::ConvertDotAddress(const string& strText)
{
	vector<string> vecdata;
	size_t pos = 0, iStart = 0;
	while (pos!=strText.npos)
	{
		pos = strText.find(',', iStart);
		string str = strText.substr(iStart, pos-iStart);
		vecdata.emplace_back(str);
		iStart = pos+1;
	}

	string strAddr = vecdata[0] + "." + vecdata[1] + "." + vecdata[2] + "." + vecdata[3];
	m_dwRemoteAddr = inet_addr(strAddr.c_str());
	m_nRemotePort = 256 * stoi(vecdata[4]) + stoi(vecdata[5]);

	return TRUE;
}

UINT CFtpIoContext::FileListToString(string& szBuff, BOOL bDetails)
{
	FILE_INFO fi[MAX_FILE_NUM] = {};
	int nFiles = GetFileList(fi, MAX_FILE_NUM, "*.*");
	char szTemp[MAX_PATH] = { 0 };
	szBuff.clear();

	if (bDetails)
	{
		for (int i = 0; i < nFiles; i++)
		{
			if (!strcmp(fi[i].szFileName, ".")) continue;
			if (!strcmp(fi[i].szFileName, "..")) continue;

			//权限
			if (fi[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				szBuff += "drwx------";
			else
				szBuff += "-rwx------";			

			//分组
			szBuff += " 1 user group ";

			//文件大小
			sprintf_s(szTemp, MAX_PATH, "% 14d", fi[i].nFileSizeLow);
			szBuff += szTemp;

			//时间
			SYSTEMTIME st;
			FileTimeToSystemTime(&fi[i].ftLastWriteTime, &st);
			sprintf_s(szTemp, MAX_PATH, " %s %02u %02u:%02u ",
				MONTHNAME[st.wMonth], st.wDay, st.wHour, st.wMinute);
			szBuff += szTemp;

			//文件名
			szBuff += fi[i].szFileName;
			szBuff += "\r\n";
		}
	}
	else
	{
		for (int i=0; i<nFiles; i++)
		{
			szBuff += fi[i].szFileName;
			szBuff += "\r\n";
		}
	}
	return szBuff.size();
}
int CFtpIoContext::GetFileList(LPFILE_INFO pFI, UINT nArraySize, const char* szPath)
{
	int idx = 0;
	char lpFileName[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, lpFileName);

	strcat_s(lpFileName, "\\");
	strcat_s(lpFileName, szPath);

	WIN32_FIND_DATA wfd = { 0 };
	HANDLE hFile = FindFirstFile(lpFileName, &wfd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do 
		{
			pFI[idx].dwFileAttributes = wfd.dwFileAttributes;
			lstrcpy(pFI[idx].szFileName, wfd.cFileName);
			pFI[idx].ftCreationTime = wfd.ftCreationTime;
			pFI[idx].ftLastWriteTime = wfd.ftLastWriteTime;
			pFI[idx].ftLastAccessTime = wfd.ftLastAccessTime;
			pFI[idx].nFileSizeHigh = wfd.nFileSizeHigh;
			pFI[idx].nFileSizeLow = wfd.nFileSizeLow;
		} while (FindNextFile(hFile, &wfd) && ++idx<nArraySize);
		FindClose(hFile);
	}
	return idx;
}

SOCKET CFtpIoContext::DataAccept(SOCKET& s)
{
	SOCKET sAccept = accept(s, NULL, NULL);
	if (sAccept != INVALID_SOCKET)
		closesocket(s);
	return sAccept;
}

int CFtpIoContext::DataSend(SOCKET s, char* buff, int nBufSize)
{
	int nBytesLeft = nBufSize;
	int idx = 0, nBytes = 0;
	while (nBytesLeft>0)
	{
		nBytes = send(s, &buff[idx], nBytesLeft, 0);
		if (nBytes == SOCKET_ERROR)
		{
			HL_PRINT(_T("Failed to send buffer to socket %d\r\n"), WSAGetLastError());
			closesocket(s);
			return -1;
		}
		nBytesLeft -= nBytes;
		idx += nBytes;
	}
	return idx;
}

char* CFtpIoContext::AbsoluteDirectory(string& szDir)
{
	auto Slash2Back = [&](string& szPath) {
		int idx = 0;
		if (szPath.empty()) return (char*)NULL;
		transform(szDir.begin(), szDir.end(), szDir.begin(), ::toupper);
		while (szPath[idx])
		{
			if ('/' == szPath[idx])
				szPath[idx] = '\\';
			idx++;
		}
		return const_cast<char*>(szPath.c_str());
	};

	char szText[MAX_PATH] = { 0 };
	strcpy_s(szText, MAX_PATH, m_szCurrDir.c_str()+2);
	if (szDir.empty())
		return NULL;

	if ('/' == szDir[0])
		strcat_s(szText, szDir.c_str());
	szDir = szText;

	return Slash2Back(szDir);
}

int CFtpIoContext::CheckDirectory(string szDir, int opt, string& szResult)
{
	replace(szDir.begin(), szDir.end(), '\\', '/');
	
	if (szDir.back() == '/')
		szDir.erase(szDir.end() - 1);

	if (szDir.empty())
	{
		if (opt == FTP_LIST)
			szDir = "/";
		else
			return ERROR_PATH_NOT_FOUND;
	}
	else
	{
		if (szDir.front() != '/')
		{
			if (m_szCurrDir.back() != '/')
				szDir = m_szCurrDir + "/" + szDir;
			else
				szDir = m_szCurrDir + szDir;
		}
	}

	//bool bPathExist = GetLocalPath(szDir, szResult);
	return 0;
}
