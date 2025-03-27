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

#define IOCP_LIST_PUSH 0
#define IOCP_LIST_POP 2
#define IOCP_LIST_EMPTY 
enum {
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};
typedef struct IocpParam {
	int nOperator;
	std::string strData;
	_beginthread_proc_type cbFunc;	//回调
	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
		nOperator = op;
		strData = sData;
		cbFunc = cb;
	}
	IocpParam() {
		nOperator = -1;
	}
}IOCP_PARAM;

void threadmain(HANDLE hIOCP) {
	std::list<std::string> lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;
	int count{}, count0{}, total{};
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
		if ((dwTransferred == 0) || (CompletionKey == NULL)) {
			TRACE("thread is prepare to exit!\r\n");
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
			count++;
		}
		else if (pParam->nOperator == IocpListPop) {
			std::string Str;
			if (lstString.size() > 0) {
				Str = lstString.front();
				lstString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(&Str);
			}
			count0++;
		}
		else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}
		delete pParam;
		printf("total %d\r\n", ++total);
	}
	//lstString.clear();
	printf("thread eixt count %d count0 %d\r\n", count, count0);
}

void threadQueueEntry(HANDLE hIOCP) {
	threadmain(hIOCP);
	_endthread();
}

void func(void* arg) {
	std::string* pstr = (std::string*)arg;
	if (pstr->size() > 0) {
		printf("pop from list:%s\r\n", pstr->c_str());
		//delete pstr;
	}
	else {
		printf("list is empty, no data!\r\n");
	}
}

int main()
{
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

	if (!CTool::Init()) return 1;

	printf("press any key to exit...\r\n");
	CYondQueue<std::string> lstStrings;
	ULONGLONG tick0 = GetTickCount64(), tick = GetTickCount64();
	while (_kbhit() == 0) {		//完成端口 把请求和实现分离开来
		if (GetTickCount64() - tick0 > 1300) {
			lstStrings.PushBack("hello world");
			tick0 = GetTickCount64();
		}
		if (GetTickCount64() - tick > 2000) {
			std::string str;
			lstStrings.PopFront(str);
			tick = GetTickCount64();
			printf("pop from queue:%s\r\n", str.c_str());
		}
		Sleep(1);

	}
	
	printf("exit done!size:%d\r\n", lstStrings.Size());
	lstStrings.Clear();
	printf("exit done!size:%d\r\n", lstStrings.Size());
	printf("exit done!\r\n");

	return 0;
}
