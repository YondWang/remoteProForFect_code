#pragma once
#include "afxdialogex.h"


// CWatchdialog 对话框

class CWatchdialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchdialog)

public:
	CWatchdialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchdialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
};
