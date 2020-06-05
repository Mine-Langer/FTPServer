#include "stdafx.h"
#include "IOCP.h"
#include "IoContext.h"

#define TIMEOUT_MS 3000

extern CIOCP * m_pIocp;

CIoContext::CIoContext()
{
	CreateSocket();
	Init();
}

CIoContext::CIoContext(SOCKET sock) :m_sIo(sock)
{
	Init();
}


CIoContext::~CIoContext()
{
}

bool CIoContext::CreateSocket()
{
	m_sIo = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_sIo == INVALID_SOCKET)
		return false;

	return true;
}

void CIoContext::Init()
{
	const char chOpt = 1;
	setsockopt(m_sIo, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));

	unsigned long ul = 1;
	ioctlsocket(m_sIo, FIONBIO, &ul);

	m_hConThreadId = 0;
}

bool CIoContext::StartSvr(int iPort)
{
	SOCKADDR_IN svrAddr = {};
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(iPort);
	svrAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(m_sIo, (SOCKADDR*)&svrAddr, sizeof(svrAddr)))
		return false;

	if (listen(m_sIo, SOMAXCONN))
		return false;

	if (!AssociateWithIocp())
		return false;

	//获取函数指针
	if (!GetExtensionFunctionPointer())
		return false;
	
	for (int i = 0; i < 100; i++)
	{
		//投递ACCEPTEX
		AsyncAcceptEx();
	}

	return true;
}

bool CIoContext::AssociateWithIocp()
{
	HANDLE hIocp = CreateIoCompletionPort((HANDLE)m_sIo, m_pIocp->m_hCompletionPort, (DWORD)this, 0);
	return (hIocp == m_pIocp->m_hCompletionPort);
}

bool CIoContext::GetExtensionFunctionPointer()
{
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	GUID GuidTransmitFile = WSAID_TRANSMITFILE;
	DWORD dwBytes = 0;

	if (SOCKET_ERROR == WSAIoctl(m_sIo, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx), &m_lpfnAcceptEx, sizeof(m_lpfnAcceptEx), &dwBytes, NULL, NULL))
	{
		HL_PRINT(_T("WSAIoctl未能获取AcceptEx函数指针。错误码：%d\n"), WSAGetLastError());
		return false;
	}

	if (SOCKET_ERROR == WSAIoctl(m_sIo, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockAddrs, sizeof(GuidGetAcceptExSockAddrs), &m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs), &dwBytes, NULL, NULL))
	{
		HL_PRINT(_T("WSAIoctl未能获取GuidGetAcceptExSockAddrs函数指针。错误码：%d\n"), WSAGetLastError());
		return false;
	}

	if (SOCKET_ERROR == WSAIoctl(m_sIo, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidTransmitFile, sizeof(GuidTransmitFile), &m_lpfnTransmitFile, sizeof(m_lpfnTransmitFile), &dwBytes, NULL, NULL))
	{
		HL_PRINT(_T("WSAIoctl未能获取GuidGetAcceptExSockAddrs函数指针。错误码：%d\n"), WSAGetLastError());
		return false;
	}

	return true;
}

bool CIoContext::AsyncAcceptEx()
{
	SOCKET	skAccept;
	skAccept = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (skAccept == INVALID_SOCKET)
		return false;

	DWORD dwByteRecved = 0;
	CIoBuffer * pIoBuffer = new CIoBuffer(IOAccepted);
	pIoBuffer->Reserve(sizeof(SOCKET) + (sizeof(SOCKADDR_IN) + 16) * 2);
	pIoBuffer->AddData(skAccept);

	BOOL bRet = m_lpfnAcceptEx(m_sIo, skAccept, pIoBuffer->m_vtBuffer + sizeof(SOCKET), 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwByteRecved, (LPOVERLAPPED)(&pIoBuffer->m_ol));
	if (bRet || WSAGetLastError() == ERROR_IO_PENDING)
		return true;
	
	delete pIoBuffer;
	return false;
}


void CIoContext::ProcessIoMsg(CIoBuffer* pIoBuffer)
{
	switch (pIoBuffer->m_nType)
	{
	case IOAccepted:
		AsyncAcceptEx();
		OnAccepted(pIoBuffer);
		break;

	case IORecved:
		if (pIoBuffer->m_nIoSize == 0)
		{
			delete pIoBuffer;
			OnClosed();
			return;
		}
		if (!AsyncRecv())
		{
			OnClosed();
			return;
		}
		OnRecved(pIoBuffer);
		break;

	case IOSent:
		OnSent(pIoBuffer);
		break;
	case IOFileTransmitted:

		break;
	default:
		delete pIoBuffer;
	}
}

void CIoContext::OnIoFailed(CIoBuffer* pIoBuffer)
{
	switch (pIoBuffer->m_nType)
	{
	case IOAccepted:
		break;
	case IOFileTransmitted:
		break;
	case IOSent:
		break;
	case IORecved:
		break;
	}
	delete pIoBuffer;
}

void CIoContext::OnAccepted(CIoBuffer* pIoBuffer)
{
	if (pIoBuffer == NULL)
		return;

	CIoContext* pIoContext = new CIoContext(*reinterpret_cast<SOCKET*>(pIoBuffer->m_vtBuffer));
	setsockopt(pIoContext->m_sIo, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_sIo, sizeof(m_sIo));

	SOCKADDR* remote = NULL;
	SOCKADDR* local = NULL;
	int remote_len = sizeof(SOCKADDR_IN);
	int local_len = sizeof(SOCKADDR_IN);
	DWORD dwRet = 0;
	m_lpfnGetAcceptExSockAddrs(pIoBuffer->m_vtBuffer + sizeof(SOCKET), 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, &local, &local_len, &remote, &remote_len);

	CopyMemory(&pIoContext->m_addrRemote, remote, sizeof(SOCKADDR_IN));
	delete pIoBuffer;

	pIoContext->AssociateWithIocp();
	pIoContext->AsyncRecv();
}

bool CIoContext::AsyncRecv()
{
	DWORD dwIoSize = 0;
	ULONG ulFlags = MSG_PARTIAL;
	CIoBuffer* pIoBuffer = new CIoBuffer(IORecved);
	pIoBuffer->Reserve(MAX_RECV_SIZE);

	int nRet = WSARecv(m_sIo, &pIoBuffer->m_wsaBuf, 1, &dwIoSize, &ulFlags, &pIoBuffer->m_ol, NULL);
	if (nRet == 0 || WSAGetLastError() == WSA_IO_PENDING)
		return true;

	HL_PRINT(_T("投递WSARecv错误，错误码：%d.\r\n"), WSAGetLastError());
	delete pIoBuffer;
	return false;
}

void CIoContext::OnClosed()
{

}

void CIoContext::ProcessJob(CIoBuffer* pIoBuffer)
{

}

void CIoContext::OnRecved(CIoBuffer* pIoBuffer)
{
	pIoBuffer->m_vtBuffer[pIoBuffer->m_nIoSize] = 0;
	m_pIocp->AddJob(CJob(this, pIoBuffer));
}

bool CIoContext::AsyncSend(CIoBuffer* pIoBuffer)
{
	DWORD dwSize = 0;
	ULONG ulFlags = MSG_PARTIAL;

	pIoBuffer->m_nType = IOSent;
	pIoBuffer->m_wsaBuf.buf = pIoBuffer->m_vtBuffer;
	pIoBuffer->m_wsaBuf.len = pIoBuffer->m_nLen;

	int nRet = WSASend(m_sIo, &pIoBuffer->m_wsaBuf, 1, &dwSize, ulFlags, &pIoBuffer->m_ol, NULL);
	if (nRet == 0 || WSAGetLastError() == WSA_IO_PENDING)
		return true;

	HL_PRINT(_T("WSASend failed, error code:%d.\r\n"), WSAGetLastError());

	delete pIoBuffer;
	return false;
}

void CIoContext::OnSent(CIoBuffer* pIoBuffer)
{
	delete pIoBuffer;
}
