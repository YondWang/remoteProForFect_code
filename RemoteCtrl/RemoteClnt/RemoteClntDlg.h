
// RemoteClntDlg.h : header file
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1)		//发送数据包的消息

// CRemoteClntDlg dialog
class CRemoteClntDlg : public CDialogEx
{
	// Construction
public:
	CRemoteClntDlg(CWnd* pParent = nullptr);	// standard constructor

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLNT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	bool isFull() const {
		return m_isFull;
	}
	CImage& getImage() {
		return m_image;
	}
	void setImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
private:
	CImage m_image;		//缓存
	bool m_isFull;		//是否有数据		true有	false没有
	bool m_isClosed;	//监视是否结束

private:
	static void threadEntryForWatchData(void* arg);		//静态函数不能用this指针
	void threadWatchData();								//成员函数可以

	static void threadEntryDownFile(void* arg);
	void threadDownFile();

	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileInfo();
	/*1 查看磁盘分区
	2 查看指定目录下的文件
	3 打开文件
	4 下载文件
	5 鼠标操作
	6 发送屏幕内容
	7 锁机
	8 解锁
	9 删除文件
	2001 测试连接
	ret: 命令号，小于0错误	*/
	int SendCommandPack(int nCmd, bool bAutoClose = true, BYTE* pData = nullptr, size_t nLength = 0);
	// Implementation
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	void LoadFileCurrent();
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeletefile();
	afx_msg void OnRunfile();

	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM LParam);
	afx_msg void OnBnClickedBtnStartwatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
