
// RemoteClntDlg.h : header file
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER + 2)
#endif // !WM_SEND_PACK_ACK

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

private:
	bool m_isClosed;	//监视是否结束

private:
	void DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam);
	void InitUIData();
	void LoadFileCurrent();
	void Str2Tree(const std::string& driver, CTreeCtrl& tree);
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParam);
	void UpdateDownloadFile(const std::string& strData, FILE* pFile);
	void LoadFileInfo();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	
	// Implementation
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

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

	
	afx_msg void OnBnClickedBtnStartwatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnEnChangeEditport();
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);

};
