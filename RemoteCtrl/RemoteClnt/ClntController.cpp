#include "pch.h"
#include "ClntController.h"

std::map<UINT, CClntController::MSGFUNC> CClntController::m_mapFunc;
CClntController* CClntController::m_instance = NULL;
CClntController::CHelper CClntController::m_helper;

CClntController* CClntController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClntController();
		TRACE("CClntController size is %d\r\n", sizeof(*m_instance));
		struct { UINT nMsg;
		MSGFUNC func;
		}MsgFuncs[] = {
		{WM_SHOW_STATUS, &CClntController::OnShowStatus},
		{WM_SHOW_WATCH, &CClntController::OnShowWatcher},
		{(UINT)-1, NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg,
								MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClntController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClntController::streadEntry,
		this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClntController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClntController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL) return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM) & msg, (LPARAM)hEvent);
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
}

bool CClntController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, 
	BYTE* pData, size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstence();
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	return ret;
}

void CClntController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成！！"));
}

int CClntController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		CString strLocal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件！或文件无法创建!"));
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		//m_hThreadDownload = (HANDLE)_beginthread(&CClntController::threadDownloadEntry, 0, this);
		/*if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
			return -1;
		}*/
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText("命令正在执行中!!");
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClntController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	m_hThreadWatch = (HANDLE)_beginthread(&CClntController::threadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClntController::threadWatchScreen()
{
	Sleep(30);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			if (GetTickCount64() - nTick < 200) {
				Sleep(50 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();

			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO:添加消息响应函数WM_SEND_PACK_ACK
			//TODO:控制发送频率
			if (ret == 1) {
				//TRACE("成功发送请求图片！\r\n");
			}
			else {
				TRACE("获取图片失败！！ ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
}

void CClntController::threadWatchScreenEntry(void* arg)
{
	CClntController* thiz = (CClntController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClntController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile == NULL) {
		AfxMessageBox(_T("本地没有权限保存该文件！或文件无法创建!"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClnt = CClientSocket::getInstence();
	do {
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		long long nLenth = *(long long*)pClnt->GetPacket().strData.c_str();
		if (nLenth == 0) {
			AfxMessageBox("文件长度为0，或无法读取文件！");
			break;
		}
		long long nCount = 0;

		while (nCount < nLenth) {
			int ret = pClnt->DealCommand();
			if (ret < 0) {
				AfxMessageBox("传输失败!");
				TRACE("传输失败！ret = %d\r\n", ret);
				break;
			}
			fwrite(pClnt->GetPacket().strData.c_str(), 1, pClnt->GetPacket().strData.size(), pFile);
			nCount += pClnt->GetPacket().strData.size();
		}

	} while (false);
	fclose(pFile);
	pClnt->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！！"), _T("完成！！"));

}

void CClntController::threadDownloadEntry(void* arg)
{
	CClntController* thiz = (CClntController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClntController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}

unsigned _stdcall CClntController::streadEntry(void* arg)
{
	CClntController* thiz = (CClntController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

LRESULT CClntController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClntController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
