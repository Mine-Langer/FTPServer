#pragma once

#include <atlcoll.h>
#include "IoDefine.h"

#define HL_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define HL_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define HL_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define HL_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}
#define HL_PRINT(...)		{ TCHAR szBuffer[1024]={0}; _stprintf_s(szBuffer, 1024 ,__VA_ARGS__); \
							  WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),szBuffer,_tcslen(szBuffer),NULL,NULL); }
//--------------------
// IO ²Ù×÷¶¯Ì¬»º³åÇø
//--------------------

class CIoBuffer
{
public:

	CIoBuffer();
	CIoBuffer(int nIoType);
	virtual ~CIoBuffer();

	void AddData(const char* pData, UINT nSize);
	void AddData(UINT uData);
	void AddData(const void* pData, UINT nSize);
	void AddData(const string& str);
	void AddData(CIoBuffer& ioBuffer);

	void Reserve(UINT len);

protected:
	void Init();

public:
	OVERLAPPED	m_ol;
	int m_nType;
	UINT m_nIoSize;
	bool m_bKeepAlive;
	char* m_vtBuffer;
	UINT m_nLen, m_nMaxLen;
	WSABUF m_wsaBuf;

};

