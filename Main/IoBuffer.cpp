#include "stdafx.h"
#include "IoBuffer.h"


CIoBuffer::CIoBuffer()
{
	Init();
}

CIoBuffer::CIoBuffer(int nIoType)
{
	Init();
	m_nType = nIoType;
}


CIoBuffer::~CIoBuffer()
{
	if (m_vtBuffer)
	{
		HL_SAFEFREE(m_vtBuffer);
	}
}

void CIoBuffer::Init()
{
	m_vtBuffer = NULL;
	m_nType = -1;
	m_nIoSize = 0;
	m_bLoggedIn = false;
	m_nLen = 0;
	m_nMaxLen = 0;

	ZeroMemory(&m_ol, sizeof(OVERLAPPED));
}


void CIoBuffer::AddData(const char* pData, UINT nSize)
{
	if (nSize > 0 && pData != NULL)
	{
		if (m_vtBuffer == NULL)
		{
			m_vtBuffer = (char*)HL_CALLOC(nSize);
			m_nMaxLen = nSize;
		}
		else
		{
			if (m_nLen + nSize > m_nMaxLen)
			{
				m_vtBuffer = (char*)HL_CREALLOC(m_vtBuffer, m_nLen + nSize);
				m_nMaxLen = m_nLen + nSize;
			}
		}

		/*for (UINT i = 0; i < nSize; i++)
		{
			m_vtBuffer[m_nLen + i] = pData[i];
		}*/
		CopyMemory(m_vtBuffer + m_nLen, pData, nSize);
		m_nLen += nSize;
	}
}

void CIoBuffer::AddData(UINT uData)
{
	AddData(reinterpret_cast<const char*>(&uData), sizeof(UINT));
}

void CIoBuffer::AddData(const void* pData, UINT nSize)
{
	AddData((const char*)pData, nSize);
}

void CIoBuffer::AddData(const string& str)
{
	AddData(str.c_str(), str.size());
}

void CIoBuffer::AddData(CIoBuffer& ioBuffer)
{
	AddData(ioBuffer.m_vtBuffer, ioBuffer.m_nLen);
}

void CIoBuffer::Reserve(UINT len)
{
	if (m_vtBuffer == NULL)
		m_vtBuffer = (char*)HL_CALLOC(len + 1);
	else
		m_vtBuffer = (char*)HL_CREALLOC(m_vtBuffer, len + 1);

	m_nMaxLen = len + 1;
	m_wsaBuf.buf = m_vtBuffer;
	m_wsaBuf.len = len;
}

//////////////////////////////////////////////////////////////////////////

