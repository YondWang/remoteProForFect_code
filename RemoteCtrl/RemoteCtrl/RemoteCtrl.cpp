﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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
