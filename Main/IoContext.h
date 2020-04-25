#pragma once
#include "IoBuffer.h"

//-------------------------------
// 每个客户端分配一个上下文内容
//-------------------------------
class CIOCP;
class CIoContext
{
public:
	CIoContext();
	CIoContext(SOCKET sock);
	virtual ~CIoContext();

	bool StartSvr(int iPort);

	virtual void ProcessIoMsg(CIoBuffer* pIoBuffer);
	virtual void ProcessJob(CIoBuffer* pIoBuffer);

	virtual void OnIoFailed(CIoBuffer* pIoBuffer);

	bool AssociateWithIocp();
	
//protected:

	bool CreateSocket();
	void Init();

	bool GetExtensionFunctionPointer();


	bool AsyncAcceptEx();
	virtual void OnAccepted(CIoBuffer* pIoBuffer);

	bool AsyncRecv();
	void OnRecved(CIoBuffer* pIoBuffer);

	bool AsyncSend(CIoBuffer* pIoBuffer);
	void OnSent(CIoBuffer* pIoBuffer);

	bool AsyncTransmitFile(const char* strFilePath, bool bKeepAlive = true);
	bool AsyncTransmitFile(HANDLE hFile, DWORD dwSize = 0, bool bKeepAlive = true);
	void OnFileTransmitted(CIoBuffer* pIoBuffer);

	void OnClosed();

public:
	SOCKET	m_sIo;
	SOCKADDR_IN	m_addrRemote;
	int m_nPort;

	LPFN_ACCEPTEX	m_lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS	m_lpfnGetAcceptExSockAddrs;

private:
	HANDLE	m_hConThreadId;
	
};

class CJob
{
public:
	CIoContext* pIoContext;
	CIoBuffer*	pIoBuffer;

	CJob() {};

	CJob(CIoContext* pContext, CIoBuffer* pBuffer) :pIoContext(pContext), pIoBuffer(pBuffer) {};

	inline bool IsNull() { return pIoContext == NULL; }
	inline bool IsNullIoBuffer() { return pIoBuffer == NULL; }
};