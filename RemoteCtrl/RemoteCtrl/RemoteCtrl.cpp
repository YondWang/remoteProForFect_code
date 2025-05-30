// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "CTool.h"
#include "Command.h"
#include <direct.h>
#include <atlimage.h>
#include "Command.h"
#include <conio.h>
#include "CYondQueue.h"
#include <MSWSock.h>
#include "CYondServer.h"

//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define INVOKE_PATH _T("C:\\Users\\yond_wang\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象
//main branch
CWinApp theApp;

using namespace std;


static bool ChooseAutoInvoke(const CString strPath) {
	TCHAR wcsSystem[MAX_PATH] = _T("");
	if (PathFileExists(strPath)) {
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法用途！！\n");
	strInfo += _T("继续运行该程序，将使得这台计算机处于被监控状态!\n");
	strInfo += _T("如果你不需要这样，请按取消按钮退出程序!\n");
	strInfo += _T("按下是按钮，该程序将被复制到你的机器上，并随系统启动而自动运行!\n");
	strInfo += _T("按下否按钮，程序只运行本次，不会在系统重留下任何东西！\n");

	int ret = MessageBox(NULL, strInfo, _T("警告！"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		//WriteRegisterTable();
		if (CTool::WriteStartupDir(strPath)) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) {
		return false;
	}
	return true;
}

void iocp();

void udp_server();
void udp_client(bool ishost = true);

void InitSocket() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void clearsock() {
	WSACleanup();
}

int main(int argc, char* argv[])
{


	if (!CTool::Init()) return 1;
	InitSocket();

	if (argc == 1) {
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID：%d \r\n", pi.dwProcessId);
			TRACE("线程ID：%d \r\n", pi.dwThreadId);
			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID：%d \r\n", pi.dwProcessId);
				TRACE("线程ID：%d \r\n", pi.dwThreadId);
				udp_server();	//服务器
			}
		}
	}
	else if (argc == 2) {	//就是主客户端
		udp_client();
	}
	else {					//从客户端
		udp_client(false);
	}
	clearsock();

	//iocp();

	/*if (CTool::IsAdmin()) {
	if (!CTool::Init()) return 1;
	if (ChooseAutoInvoke(INVOKE_PATH)) {
		CCommand cmd;
		int ret = CServerSocket::getInstence()->Run(&CCommand::RunCommand, &cmd);
		switch (ret) {
		case -1:
			MessageBox(NULL, _T("网络初始化，请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
			break;
		case -2:
			MessageBox(NULL, _T("重试超时"), _T("请稍后再试！"), MB_OK | MB_ICONERROR);
			break;
			}
		}
	}
	else if (CTool::RunAsAdmin() == false) {
			CTool::ShowError();
			return 1;
	}*/

	return 0;
}

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped() {
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		memset(&m_buffer, 0, sizeof(m_buffer));
	}
};

void iocp() {
	CYondServer server;
	server.StartService();
	getchar();
}

void udp_server() {
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("%s(%d):%s ERROR!!!!(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return;
	}
	std::list<sockaddr_in>lstclients;
	sockaddr_in server, client;
	memset(&server, 0, sizeof(sockaddr_in));
	memset(&client, 0, sizeof(sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (-1 == bind(sock, (sockaddr*)&server, sizeof(sockaddr_in))) {
		printf("%s(%d):%s ERROR!!!!(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		closesocket(sock);
		return;
	}
	std::string buf;
	buf.resize(1024 * 256);
	memset((char*)buf.c_str(), 0, buf.size());
	int len = sizeof(client);
	int ret = 0;
	while (!_kbhit()) {
		ret = recvfrom(sock, (char*)buf.c_str(), sizeof(buf), 0, (sockaddr*)&client, &len);
		if (ret > 0) {
			if (lstclients.size() <= 0) {
				lstclients.push_back(client);
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				ret = sendto(sock, buf.c_str(), ret, 0, (sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			else {
				memcpy((void*)buf.c_str(), &lstclients.front(), sizeof(lstclients.front()));
				ret = sendto(sock, buf.c_str(), sizeof(lstclients.front()), 0, (sockaddr*)&client, len);
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			//CTool::Dump((BYTE*)buf.c_str(), ret);
		}
		else {
			printf("%s(%d):%s ERROR(%d)!!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
		}
	}
	closesocket(sock);
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
}
void udp_client(bool ishost) {
	Sleep(2000);
	sockaddr_in server, client;
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	int len = sizeof(client);
	if (sock == INVALID_SOCKET) {
		printf("%s(%d):%s ERROR!!!!\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	if (ishost) {	//主客户端代码
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world!!!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				printf("%s(%d):%s: %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
			}
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("host %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				printf("%s(%d):%s: %s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}
	else {			//从客户端代码
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world!!!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
		printf("%s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
			printf("client %s(%d):%s ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(addr));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, client.sin_port);
				printf("%s(%d):%s: %d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.size());
				printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs((paddr->sin_port)));
				msg = "im client!!\r\n";
				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
				printf("%s(%d):%s ERROR(%d)!!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			}
		}
	}
	closesocket(sock);
}