#pragma once
#include "IoBuffer.h"

//-------------------------------
// 每个客户端分配一个上下文内容
//-------------------------------

class CIoContext
{
public:
	CIoContext();
	~CIoContext();
};

