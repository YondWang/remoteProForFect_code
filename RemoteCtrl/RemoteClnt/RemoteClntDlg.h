
// RemoteClntDlg.h : header file
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1)		//�������ݰ�����Ϣ

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
	CImage m_image;		//����
	bool m_isFull;		//�Ƿ�������		true��	falseû��
	bool m_isClosed;	//�����Ƿ����

private:
	static void threadEntryForWatchData(void* arg);		//��̬����������thisָ��
	void threadWatchData();								//��Ա��������

	static void threadEntryDownFile(void* arg);
	void threadDownFile();

	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileInfo();
	/*1 �鿴���̷���
	2 �鿴ָ��Ŀ¼�µ��ļ�
	3 ���ļ�
	4 �����ļ�
	5 ������
	6 ������Ļ����
	7 ����
	8 ����
	9 ɾ���ļ�
	2001 ��������
	ret: ����ţ�С��0����	*/
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
	// ��ʾ�ļ�
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeletefile();
	afx_msg void OnRunfile();

	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM LParam);
	afx_msg void OnBnClickedBtnStartwatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
