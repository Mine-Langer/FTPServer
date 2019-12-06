#pragma once

#include <Winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#include <process.h>
#include <time.h>
#pragma warning(disable:4786) 
#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include <map>
#include <queue>
#include <algorithm>
#include <set>
#include <list>
using namespace std;

enum IOType
{
	IOConnected,
	IOAccepted,
	IORecved,
	IOSent,
	IOWriteCompleted,
	IOFileTransmitted,
	IOFailed,
};


#define MAX_RECV_SIZE 40960
#define FLAG_CLOSING 0x19844891

typedef map<string, string>	MIMETYPES;

#define FTP_SERVER_NAME "FtpSvr 1.00 - Langer"
#define MAX_FTP_REQUEST 40960

#define FTP_DEFAULT_FILE "index.html"