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
public:
	int m_nObjWidth;
	int m_nObjHeight;
	CImage m_image;
protected:
	//是否有数据 true有 false没有
	bool m_isFull;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	
	DECLARE_MESSAGE_MAP()
public:
	CImage& getImage() {
		return m_image;
	}
	bool isFull() const {
		return m_isFull;
	}
	void setImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
	CPoint UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen = false);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();
};
