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
};

