#pragma once
#include "ClientSocket.h"
#include "CWatchdialog.h"
#include "RemoteClntDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include <map>
#include "CTool.h"

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
	//更新网络服务器的地址
	void UpdateAddress(int nIP, int nPort) {
		CClientSocket::getInstence()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstence()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstence()->CloseSocket();
	}
	bool SendPacket(CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstence();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = nullptr, size_t nLength = 0);
	int GetImage(CImage& image) {
		CClientSocket* pClnt = CClientSocket::getInstence();
		return CTool::Bytes2Image(image, pClnt->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	CClntController() : 
		m_statusDlg(&m_remoteDlg), 
		m_watchDlg(&m_remoteDlg) 
	{
		m_isClosed = true;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;

	}
	~CClntController() {

		WaitForSingleObject(m_hThread, 100);

	}
	void threadFunc();
	static unsigned _stdcall streadEntry(void* arg);
	static void releaseInstance() {
		TRACE("CClntController has called!\r\n");

		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
			TRACE("CClntController has released!\r\n");
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
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;

	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
	unsigned m_nThreadID;

	bool m_isClosed;	//监视是否结束

	static CClntController* m_instance;

public:
	class CHelper {
	public:
		CHelper() {
			//CClntController::getInstance();

		}
		~CHelper() {
			CClntController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

