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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象
//main branch
CWinApp theApp;

using namespace std;

void ChooseAutoInvoke() {
	CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	CString strInfo = _T("该程序只允许用于合法用途！！\n");
	strInfo += _T("继续运行该程序，将使得这台计算机处于被监控状态!\n");
	strInfo += _T("如果你不需要这样，请按取消按钮退出程序!\n");
	strInfo += _T("按下是按钮，该程序将被复制到你的机器上，并随系统启动而自动运行!\n");
	strInfo += _T("按下否按钮，程序只运行本次，不会在系统重留下任何东西！\n");

	int ret = MessageBox(NULL, strInfo, _T("警告！"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	if (ret == IDYES) {
		char sPath[MAX_PATH] = "";
		char sSys[MAX_PATH] = "";
		std::string strExe = "\\RemoteCtrl.exe ";

		GetCurrentDirectoryA(MAX_PATH, sPath);
		GetSystemDirectoryA(sSys, sizeof(sSys));
		std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
		ret = system(strCmd.c_str());
		TRACE("ret = %d\r\n", ret);
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机失败！是否权限不足？0x00"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			exit(0);
		}
		CString strPath = CString(_T("%SystemRoot%\\SysWOW64\\RemoteCtrl.exe"));
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机失败！是否权限不足？0x01"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			exit(0);
		}
		RegCloseKey(hKey);
	}
	else if (ret == IDCANCEL) {
		exit(0);
	}
	return;
}

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// 初始化 MFC 并在失败时显示错误  
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			nRetCode = 1;
		}
		else
		{
			CCommand cmd;
			ChooseAutoInvoke();
			int ret = CServerSocket::getInstence()->Run(&CCommand::RunCommand, &cmd);
			switch (ret)
			{
			case -1:
				MessageBox(NULL, _T("网络初始化，请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			case -2:
				MessageBox(NULL, _T("重试超时"), _T("请稍后再试！"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			default:
				break;
			}
		}
	}
	else
	{
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}
