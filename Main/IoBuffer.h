#pragma once

#include <atlcoll.h>
#include "IoDefine.h"

#define HL_ALLOC(sz)		HeapAlloc(GetProcessHeap(),0,sz)
#define HL_CALLOC(sz)		HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define HL_CREALLOC(p,sz)	HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)
#define HL_SAFEFREE(p)		if(NULL != p){HeapFree(GetProcessHeap(),0,p);p=NULL;}

//--------------------
// IO ²Ù×÷¶¯Ì¬»º³åÇø
//--------------------

class CIoBuffer
{
public:
	CIoBuffer();
	~CIoBuffer();
};

