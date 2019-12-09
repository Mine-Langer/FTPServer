#pragma once
#include "IoContext.h"

class CFtpIoContext :public CIoContext
{
public:
	CFtpIoContext(SOCKET sock);
	virtual ~CFtpIoContext();

	virtual void ProcessJob(CIoBuffer* pIoBuffer);

	void FtpProcessRequest(const string& strRequest);
	
	bool Welcome();
};

