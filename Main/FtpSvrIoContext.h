#pragma once
#include "IoContext.h"

class CFtpSvrIoContext :public CIoContext
{
public:
	CFtpSvrIoContext();
	virtual ~CFtpSvrIoContext();

	void OnAccepted(CIoBuffer* pIoBuffer);
	void OnClosed();

	void SetRootPath(const char* szPath);

protected:
	bool Welcome();

private:
	wstring	m_szRootPath;
};

