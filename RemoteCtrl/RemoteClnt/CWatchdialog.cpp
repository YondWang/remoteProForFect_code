// CWatchdialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClnt.h"
#include "afxdialogex.h"
#include "CWatchdialog.h"
#include "RemoteClntDlg.h"


// CWatchdialog 对话框

IMPLEMENT_DYNAMIC(CWatchdialog, CDialog)

CWatchdialog::CWatchdialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchdialog::~CWatchdialog()
{
}

void CWatchdialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWatchdialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CWatchdialog 消息处理程序


BOOL CWatchdialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 50, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchdialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClntDlg* pParent = (CRemoteClntDlg*)GetParent();
		if (pParent->isFull()) {

		}
	}
	CDialog::OnTimer(nIDEvent);
}
