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

#define FTP_PORT        21     // FTP 控制端口
#define DATA_FTP_PORT   20     // FTP 数据端口
#define DATA_BUFSIZE    8192
#define MAX_NAME_LEN    128
#define MAX_PWD_LEN     128
#define MAX_RESP_LEN    1024
#define MAX_REQ_LEN     256
#define MAX_ADDR_LEN    80

#define WSA_RECV         0
#define WSA_SEND         1

#define USER_OK         331
#define LOGGED_IN       230
#define LOGIN_FAILED    530
#define CMD_OK          200
#define OPENING_AMODE   150
#define TRANS_COMPLETE  226
#define CANNOT_FIND     550
#define FTP_QUIT        221
#define CURR_DIR        257
#define DIR_CHANGED     250
#define OS_TYPE         215
#define REPLY_MARKER    504
#define PASSIVE_MODE    227

#define FTP_DOWNLOAD	1
#define FTP_UPLOAD		2
#define FTP_RENAME		3
#define FTP_DELETE		4
#define FTP_CREATE_DIR	5
#define FTP_LIST		6

#define DEFAULT_USER		"admin"
#define DEFAULT_PASS		"admin"
#define MAX_FILE_NUM        1024

#define MODE_PORT       0
#define MODE_PASV       1

#define PORT_BIND   1821

enum
{
	STATUS_IDLE = 0,
	STATUS_LOGIN = 1,
	STATUS_LIST = 2,
	STATUS_UPLOAD = 3,
	STATUS_DOWNLOAD = 4,
};