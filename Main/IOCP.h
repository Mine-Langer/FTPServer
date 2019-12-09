#pragma once
#include "IoContext.h"
//---------------------
// 完成端口服务器
//---------------------
class CIOCP
{
public:
	CIOCP();
	virtual ~CIOCP();

	BOOL Start();
	void Stop();
	void CloseHandles();

	void StartThreads();
	void StopThreads();

	BOOL CreateCompletionPorts();

	static unsigned int WINAPI ThreadFreeClient(LPVOID pParam);
	static unsigned int WINAPI ThreadIoWorker(LPVOID pParam);
	static unsigned int WINAPI ThreadJob(LPVOID pParam);

	CJob GetJob();
	void ProcessJob();
	void AddJob(CJob Job);
	void AddCloseClient(CIoContext* pIoContext);
	void WakenOneJobThread();

public:
	HANDLE	m_hCompletionPort;
	HANDLE	m_hJobCompletionPort;
	HANDLE	m_hFreeClientCompletionPort;

	deque<CJob> m_dqJobs;
	HANDLE	m_hEventThreadStopped;
	LONG	m_nThread;
	int		m_nThreadJob, m_nThreadIo;

	CRITICAL_SECTION	m_csJob, m_csClosedClient;
	deque<CIoContext*>	m_dqClosedClient;
	
};

