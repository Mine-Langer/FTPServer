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
	FtpProcessRequest(pIoBuffer->m_vtBuffer);
}

void CFtpIoContext::FtpProcessRequest(const string& strRequest)
{

}

bool CFtpIoContext::Welcome()
{
	char* szWelcome = "220 »¶Ó­µÇÂ¼µ½FTP Server  -- By:Langer   \r\n";
	CIoBuffer* pBuffer = new CIoBuffer();
	pBuffer->AddData(szWelcome, strlen(szWelcome));

	return AsyncSend(pBuffer);
}