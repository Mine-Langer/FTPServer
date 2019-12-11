#include "stdafx.h"
#include "FtpSvrIoContext.h"
#include "FtpIoContext.h"

CFtpSvrIoContext::CFtpSvrIoContext()
{
}


CFtpSvrIoContext::~CFtpSvrIoContext()
{
}


void CFtpSvrIoContext::OnAccepted(CIoBuffer* pIoBuffer)
{
	if (pIoBuffer == NULL)
		return;

	CFtpIoContext * pIoContext = new CFtpIoContext(*reinterpret_cast<SOCKET*>(pIoBuffer->m_vtBuffer));

	setsockopt(pIoContext->m_sIo, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_sIo, sizeof(m_sIo));

	SOCKADDR *remote, *local;
	int remote_len = sizeof(SOCKADDR_IN);
	int local_len = sizeof(SOCKADDR_IN);
	DWORD dwRet = 0;

	m_lpfnGetAcceptExSockAddrs(pIoBuffer->m_vtBuffer + sizeof(SOCKET), 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, &local, &local_len, &remote, &remote_len);
	CopyMemory(&pIoContext->m_addrRemote, remote, sizeof(SOCKADDR_IN));
	
	delete pIoBuffer;

	if (!(pIoContext->AssociateWithIocp() && pIoContext->AsyncRecv()))
		pIoContext->OnClosed();
	
	bool b = pIoContext->Welcome();
}


