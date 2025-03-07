#pragma once
#include "ClientSocket.h"
#include "CWatchdialog.h"
#include "RemoteClntDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)		//发送包数据
#define WM_SEND_DATA (WM_USER + 2)		//发送数据
#define WM_SHOW_STATUS (WM_USER + 3)	//展示状态
#define WM_SHOW_WATCH (WM_USER + 4)		//远程监控
#define WM_SEND_MESSAGE (WM_USER + 1000)	//自定义消息处理

class CClntController
{
public:
	//获取全局唯一对象
	static CClntController* getInstance();
	//初始化
	int InitController();
	//启动
	int Invoke(CWnd*& pMainWnd);
	//发送消息
	LRESULT SendMessage(MSG msg);

protected:
	CClntController() : 
		m_statusDlg(&m_remoteDlg), 
		m_watchDlg(&m_remoteDlg) 
	{
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;

	}
	~CClntController() {

		WaitForSingleObject(m_hThread, 100);

	}
	void threadFunc();
	static unsigned _stdcall streadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	}MSGINFO;
	typedef LRESULT(CClntController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	std::map<UUID, MSGINFO*>m_mapMessage;
	CWatchdialog m_watchDlg;
	CRemoteClntDlg m_remoteDlg;
	CStatusDlg m_statusDlg;

	HANDLE m_hThread;
	unsigned m_nThreadID;

	static CClntController* m_instance;

public:
	class CHelper {
	public:
		CHelper() {
			CClntController::getInstance();

		}
		~CHelper() {
			CClntController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

