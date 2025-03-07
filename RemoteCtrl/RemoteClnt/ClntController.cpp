#include "pch.h"
#include "ClntController.h"

std::map<UINT, CClntController::MSGFUNC> CClntController::m_mapFunc;
CClntController* CClntController::m_instance = NULL;

CClntController* CClntController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClntController();
		struct { UINT nMsg;
		MSGFUNC func;
		}MsgFuncs[] = {
		{WM_SEND_DATA, &CClntController::OnSendPack},
		{WM_SEND_PACK, &CClntController::OnSendData},
		{WM_SHOW_STATUS, &CClntController::OnShowStatus},
		{WM_SHOW_WATCH, &CClntController::OnShowWatcher},
		{(UINT)-1, NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg,
								MsgFuncs[i].func));
		}
	}
	return nullptr;
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
	return info.result;
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

LRESULT CClntController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	return LRESULT();
}

LRESULT CClntController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{

	return LRESULT();
}

LRESULT CClntController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClntController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
