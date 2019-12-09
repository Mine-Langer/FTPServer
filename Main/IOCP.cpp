#include "stdafx.h"
#include "IOCP.h"

CIOCP * m_pIocp;

CIOCP::CIOCP()
{
	InitializeCriticalSection(&m_csJob);
	InitializeCriticalSection(&m_csClosedClient);

	m_hCompletionPort = m_hJobCompletionPort = NULL;
	m_hEventThreadStopped = CreateEvent(NULL, TRUE, TRUE, NULL);

	m_pIocp = this;

	m_nThread = 0;
	m_nThreadJob = 10;

	SYSTEM_INFO sysInfo = {};
	GetSystemInfo(&sysInfo);
	m_nThreadIo = sysInfo.dwNumberOfProcessors * 2;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}


CIOCP::~CIOCP()
{
	Stop();
	DeleteCriticalSection(&m_csClosedClient);
	DeleteCriticalSection(&m_csJob);
	CloseHandle(m_hEventThreadStopped);
	WSACleanup();
}

void CIOCP::CloseHandles()
{
	if (m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}

	if (m_hJobCompletionPort)
	{
		CloseHandle(m_hJobCompletionPort);
		m_hJobCompletionPort = NULL;
	}
}

void CIOCP::Stop()
{
	StopThreads();
	CloseHandles();
}

BOOL CIOCP::Start()
{
	if (!CreateCompletionPorts())
	{
		return FALSE;
	}

	StartThreads();

	return TRUE;
}

void CIOCP::StartThreads()
{
	int i = 0;
	for (i = 0; i < m_nThreadIo; i++)
	{
		_beginthreadex(NULL, 0, ThreadIoWorker, this, 0, NULL);
	}

	for (i = 0; i < m_nThreadJob; i++)
	{
		_beginthreadex(NULL, 0, ThreadJob, this, 0, NULL);
	}

	_beginthreadex(NULL, 0, ThreadFreeClient, this, 0, NULL);
}

void CIOCP::StopThreads()
{
	ResetEvent(m_hEventThreadStopped); 
	int i = 0;
	PostQueuedCompletionStatus(m_hFreeClientCompletionPort, 0, 0, NULL);

	for (i = 0; i < m_nThreadJob; i++)
	{
		PostQueuedCompletionStatus(m_hJobCompletionPort, 0, 0, NULL);
	}

	for (i = 0; i < m_nThreadIo; i++)
	{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, NULL);
	}

	if (m_nThread > 0)
	{
		WaitForSingleObject(m_hEventThreadStopped, 3000);
	}
}

BOOL CIOCP::CreateCompletionPorts()
{
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hCompletionPort == NULL)
		return FALSE;

	m_hJobCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hJobCompletionPort == NULL)
		return FALSE;

	m_hFreeClientCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hFreeClientCompletionPort == NULL)
		return FALSE;

	return TRUE;
}


unsigned int WINAPI CIOCP::ThreadFreeClient(LPVOID pParam)
{
	CIOCP* pThis = (CIOCP*)pParam;
	DWORD dwIoSize = 0, dwKey = 0; 
	LPOVERLAPPED lpOverlapped = NULL;
	BOOL bRet = FALSE;

	HANDLE hIocp = pThis->m_hFreeClientCompletionPort;
	InterlockedIncrement(&pThis->m_nThread);

	deque<CIoContext *> & dqCloseClient = pThis->m_dqClosedClient;
	CRITICAL_SECTION & csCloseClient = pThis->m_csClosedClient;

	int i = 0, Num = 0;

	while (TRUE)
	{
		bRet = GetQueuedCompletionStatus(hIocp, &dwIoSize, &dwKey, &lpOverlapped, 300);
		if (!bRet && dwKey == 0 && WSAGetLastError() != ERROR_TIMEOUT)
			break;

		EnterCriticalSection(&csCloseClient);
		if (Num > 0)
		{
			for (i = 0; i < Num; i++)
			{
				delete dqCloseClient[i];
			}
			dqCloseClient.erase(dqCloseClient.begin(), dqCloseClient.begin() + Num);
		}
		Num = dqCloseClient.size();
		LeaveCriticalSection(&csCloseClient);
	}

	EnterCriticalSection(&csCloseClient);
	for (i = 0; i < dqCloseClient.size(); i++)
	{
		delete dqCloseClient[i];
	}
	dqCloseClient.erase(dqCloseClient.begin(), dqCloseClient.begin() + Num);
	LeaveCriticalSection(&csCloseClient);

	InterlockedDecrement(&pThis->m_nThread);
	if (pThis->m_nThread <= 0)
	{
		SetEvent(pThis->m_hEventThreadStopped);
	}

	return 0;
}

unsigned int WINAPI CIOCP::ThreadIoWorker(LPVOID pParam)
{
	CIOCP* pThis = (CIOCP*)pParam;
	DWORD dwIoSize = 0;
	CIoContext* lpIoContext = NULL;
	CIoBuffer*	lpOverlapBuff = NULL;
	LPOVERLAPPED lpOverlapped = NULL;
	BOOL bRet = FALSE;
	HANDLE hIocp = pThis->m_hCompletionPort;

	InterlockedIncrement(&pThis->m_nThread);

	while (TRUE)
	{
		bRet = GetQueuedCompletionStatus(hIocp, &dwIoSize, (LPDWORD)&lpIoContext, &lpOverlapped, INFINITE);
		if (bRet && lpIoContext != NULL)
		{
			lpOverlapBuff = CONTAINING_RECORD(lpOverlapped, CIoBuffer, m_ol);
			lpOverlapBuff->m_nIoSize = dwIoSize;
			lpIoContext->ProcessIoMsg(lpOverlapBuff);
		}
		else if (!bRet)
		{
			lpOverlapBuff = CONTAINING_RECORD(lpOverlapped, CIoBuffer, m_ol);
			lpIoContext->OnIoFailed(lpOverlapBuff);
		}
		else
		{
			break;
		}
	}

	InterlockedDecrement(&pThis->m_nThread);

	if (pThis->m_nThread <= 0)
	{
		SetEvent(pThis->m_hEventThreadStopped);
	}

	return 0;
}

unsigned int WINAPI CIOCP::ThreadJob(LPVOID pParam)
{
	CIOCP * pThis = (CIOCP*)pParam;
	DWORD dwIoSize = 0, dwKey = 0;
	LPOVERLAPPED lpOverlapped = NULL;
	BOOL bRet = FALSE;
	HANDLE hIocp = pThis->m_hJobCompletionPort;

	InterlockedIncrement(&pThis->m_nThread);
	while (TRUE)
	{
		bRet = GetQueuedCompletionStatus(hIocp, &dwIoSize, &dwKey, &lpOverlapped, INFINITE);
		if (!bRet || dwKey == 0)
			break;

		pThis->ProcessJob();
	}
	InterlockedDecrement(&pThis->m_nThread);

	if (pThis->m_nThread <= 0)
	{
		SetEvent(pThis->m_hEventThreadStopped);
	}
	return 0;
}

void CIOCP::ProcessJob()
{
	CJob Job = GetJob();
	if (Job.IsNullIoBuffer())
		AddCloseClient(Job.pIoContext);
	else
	{
		Job.pIoContext->ProcessJob(Job.pIoBuffer);
		delete Job.pIoBuffer;
	}
}

CJob CIOCP::GetJob()
{
	CJob Job;

	EnterCriticalSection(&m_csJob);
	Job = m_dqJobs[0];
	m_dqJobs.pop_front();
	LeaveCriticalSection(&m_csJob);
	
	return Job;
}

void CIOCP::AddJob(CJob Job)
{
	EnterCriticalSection(&m_csJob);
	m_dqJobs.push_back(Job);
	LeaveCriticalSection(&m_csJob);

	WakenOneJobThread();
}

void CIOCP::WakenOneJobThread()
{
	PostQueuedCompletionStatus(m_hJobCompletionPort, 0, 1, NULL);
}

void CIOCP::AddCloseClient(CIoContext* pIoContext)
{
	EnterCriticalSection(&m_csClosedClient);
	m_dqClosedClient.push_back(pIoContext);
	LeaveCriticalSection(&m_csClosedClient);
}

