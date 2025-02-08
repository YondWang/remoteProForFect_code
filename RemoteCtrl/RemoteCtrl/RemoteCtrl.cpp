// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象
//main branch
CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize) {
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

int MakeDriverInfo() {      //driver从1开始，1是a盘（软盘），2是b盘（软盘），3是c盘...26是z盘
	std::string result;
	for (int i = 1; i <= 26; i++) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0) {
				result += ',';
			}
			result += 'A' + i - 1;
		}
	}

	CPacket pack(1, (BYTE*)result.c_str(), result.size());      //打包用的
	Dump((BYTE*)pack.Data(), pack.Size());
	//CServerSocket::getInstence()->Send(pack);
	return 0;
}

#include<io.h>
#include<list>
typedef struct file_info {
	file_info() {
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;         //是否有效
	char szFileName[256];   //文件名
	BOOL HasNext;           //0 No 1 Has
	BOOL IsDirectory;       //是否为目录， 0否1是
}FILEINFO, * PFILEINFO;

int MakeDirectorInfo() {
	//std::list<FILEINFO> lstFileInfos;
	std::string strPath;
	if (CServerSocket::getInstence()->GetFilePath(strPath) == false) {
		OutputDebugString(_T("当前命令不是获取文件列表，命令解析错误!!"));
		return -1;
	}
	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;
		finfo.IsInvalid = TRUE;
		finfo.IsDirectory = TRUE;
		finfo.HasNext = FALSE;
		memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
		//lstFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstence()->Send(pack);
		OutputDebugString(_T("没有权限访问目录！"));
		return -2;
	}
	_finddata_t fdata;
	int hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(_T("查找失败，没有找到文件!"));
		return -3;
	}
	do {
		FILEINFO finfo;
		finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		//lstFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstence()->Send(pack);
	} while (!_findnext(hfind, &fdata));
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstence()->Send(pack);

	return 0;
}

int RunFile() {
	std::string strPath;
	CServerSocket::getInstence()->GetFilePath(strPath);
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	CPacket pack(3, NULL, 0);
	CServerSocket::getInstence()->Send(pack);
	return 0;
}
#pragma warning(disable:4996)   //fopen sprintf strcpy strstr
int DownloadFile() {
	std::string strPath;
	CServerSocket::getInstence()->GetFilePath(strPath);
	long long data;
	FILE* pFile = NULL;
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
	if (err != 0) {
		CPacket pack(4, (BYTE*)&data, 8);
		CServerSocket::getInstence()->Send(pack);
		return -1;
	}
	if (pFile != NULL) {
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);
		CPacket head(4, (BYTE*)&data, 8);
		fseek(pFile, 0, SEEK_SET);
		char buffer[1024] = "";
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			CPacket pack(4, (BYTE*)buffer, rlen);
			CServerSocket::getInstence()->Send(pack);
		} while (rlen >= 1024);
		fclose(pFile);
	}
	CPacket pack(4, NULL, 0);
	CServerSocket::getInstence()->Send(pack);
	return 0;
}

int mouseEvent() {
	MOUSEEV mouse;
	if (CServerSocket::getInstence()->GetMouseEvent(mouse)) {

		DWORD nFlags = 0;
		switch (mouse.nButton) {
		case 0:     //left button
			nFlags = 1;
			break;
		case 1:     //right button
			nFlags = 2;
			break;
		case 2:     //mid button
			nFlags = 4;
			break;
		case 4:     //没有按键
			nFlags = 8;
			break;
		default:
			break;
		}
		if (nFlags != 8) {
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		}

		switch (mouse.nAction) {
		case 0:     //单击
			nFlags |= 0x10;
			break;
		case 1:     //双击
			nFlags |= 0x20;
			break;
		case 2:     //按下
			nFlags |= 0x40;
			break;
		case 3:     //放开
			nFlags |= 0x80;
			break;

		default:
			break;
		}
		switch (nFlags)
		{
		case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11://左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81://左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22://右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42://右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82://右键放开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24://中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44://中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84://中键放开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08://单纯的鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		CPacket pack(4, NULL, 0);
		CServerSocket::getInstence()->Send(pack);
	}
	else {
		OutputDebugString(_T("获取鼠标操作参数失败！"));
		return -1;
	}


	return 0;
}

int SendScreen() {
	CImage screen;
	HDC hScreen = ::GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, 2560, 1420, hScreen, 0, 0, SRCCOPY);

	ReleaseDC(NULL, hScreen);

	DWORD tick = GetTickCount64();
	screen.Save(_T("test_2025.png"), Gdiplus::ImageFormatPNG);
	TRACE("png %d", GetTickCount64() - tick);
	tick = GetTickCount64();
	screen.Save(_T("test_2025.jpg"), Gdiplus::ImageFormatJPEG);
	TRACE("jpg %d", GetTickCount64() - tick);
	screen.ReleaseDC();
	return 0;
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
			// TODO: socket,bind,listen,accept,read,write,close
			//init sock
			//CServerSocket* pserver = CServerSocket::getInstence();
			//if (pserver->InitSocket() == false) {
			//    MessageBox(NULL, _T("网络初始化，请检查网络状态"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
			//    exit(0);
			//}
			//int count = 0;
			//while (CServerSocket::getInstence() != NULL) {
			//    if (pserver->AcceptClient() == false) {
			//        if (count >= 3) {
			//            MessageBox(NULL, _T("重试超时"), _T("请稍后再试！"), MB_OK | MB_ICONERROR);
			//            exit(0);
			//        }
			//        MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
			//        count++;
			//    }
			//    int ret = pserver->DealCommand();
			//    //TODO:
			//}
			int nCmd = 6;
			switch (nCmd)
			{
			case 1:     //查看磁盘分区
				MakeDriverInfo();
				break;
			case 2:     //查看指定目录下的文件
				MakeDirectorInfo();
				break;
			case 3:     //打开文件
				RunFile();
				break;
			case 4:     //下载文件
				DownloadFile();
				break;
			case 5:     //鼠标操作
				mouseEvent();
				break;
			case 6:		//发送屏幕内容 == 发送屏幕截图
				SendScreen();
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
