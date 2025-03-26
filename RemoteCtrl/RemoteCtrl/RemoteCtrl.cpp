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
		return false;
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

int main()
{
	if (CTool::IsAdmin()) {
		if (!CTool::Init() == false) return 1;
		CCommand cmd;
		if (!ChooseAutoInvoke(INVOKE_PATH)) {
			int ret = CServerSocket::getInstence()->Run(&CCommand::RunCommand, &cmd);
			switch (ret)
			{
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
	}
	return 0;
}
