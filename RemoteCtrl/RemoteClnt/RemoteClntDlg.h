
// RemoteClntDlg.h : header file
//

#pragma once
#include "ClientSocket.h"

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

private:
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

	void LoadFIleCurrent();
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
	afx_msg void OnDownload();
	afx_msg void OnDeletefile();
	afx_msg void OnRunfile();
};
