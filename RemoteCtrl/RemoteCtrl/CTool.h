#pragma once
#include <string>
#include "pch.h"
#include "framework.h"


//开机启动时权限是跟随启动用户的，如果两者权限不一致则会导致程序启动失败
//开机启动对环境变量有影响，如果依赖dll，则可能启动失败
//TODO:复制这些dll到system32或者sysWOW64下面

/*
改bug的思路
0 观察现象
1 确定范围
2 分析错误可能性
3 测试或者打日志，排查错误
4 处理错误
5 验证/长时间验证/多次验证/多条件的验证
*/

class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
			strOut += buf;
		}
		strOut += "\n";
		//std::cout << strOut.c_str() << std::endl;
		OutputDebugStringA(strOut.c_str());
	}

	static bool Init() {		//用于带MFC命令行项目初始化，通用
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}

	

	static bool WriteStartupDir(const CString& strPath) {	//通过修改开机启动文件夹来实现开机启动
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
	}

	static void ShowError() {
		LPWSTR lpMessageBuf = NULL;
		//strerror(errno);	用于标准c语言库
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("发生错误"), 0);

		LocalFree(lpMessageBuf);
	}

	static bool IsAdmin() {
		HANDLE hToken = NULL;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == FALSE) {
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;
		}
		TRACE(_T("length of tokeninformation is %d \r\n"), len);
		return false;
	}
	
	static bool RunAsAdmin() {	//TODO:获取管理员权限、使用该权限创建进程
		//本地策略组 开启Administrator账户 禁止空密码只能登录本地控制台
		STARTUPINFO si = {};
		PROCESS_INFORMATION pi = {};
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret) {
			ShowError();		//TODO:去除调试信息
			MessageBox(NULL, sPath, _T("创建进程失败"), 0);
			return false;
		}
		WaitForSingleObject(pi.hProcess, (DWORD)INFINITY);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	static bool WriteRegisterTable(const CString& strPath) {		//通过修改注册表来实现开机启动
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret == FALSE) {
			MessageBox(NULL, _T("复制文件失败，是否权限不足\r\n"), _T("错误！"), MB_ICONERROR | MB_TOPMOST);
			::exit(0);
		}
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机失败！是否权限不足？0x00"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机失败！是否权限不足？0x01"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	

};