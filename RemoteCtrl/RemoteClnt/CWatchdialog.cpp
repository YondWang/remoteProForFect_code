// CWatchdialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClnt.h"
#include "afxdialogex.h"
#include "CWatchdialog.h"
#include "ClntController.h"


// CWatchdialog 对话框

IMPLEMENT_DYNAMIC(CWatchdialog, CDialog)

CWatchdialog::CWatchdialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjHeight = -1;
	m_nObjWidth = -1;
}

CWatchdialog::~CWatchdialog()
{
}

void CWatchdialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchdialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchdialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchdialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchdialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchdialog 消息处理程序


CPoint CWatchdialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	CRect  clntRect;
	if (!isScreen) ClientToScreen(&point);			//全局坐标到客户坐标
	m_picture.ScreenToClient(&point);
	

	//本地坐标，到远程坐标
	m_picture.GetWindowRect(clntRect);

	int width0 = clntRect.Width();
	int height0 = clntRect.Height();

	/*HDC hdc = ::GetDC(NULL);
	int width = GetDeviceCaps(hdc, HORZRES);
	int height = GetDeviceCaps(hdc, VERTRES);
	::ReleaseDC(NULL, hdc);*/

	return CPoint(point.x * m_nObjWidth / width0
		, point.y * m_nObjHeight / height0);
}

BOOL CWatchdialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_isFull = false;

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchdialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CClntController* pParent = CClntController::getInstance();
		if (m_isFull) {
			CRect rect;
			m_picture.GetWindowRect(rect);
			m_nObjWidth = m_image.GetWidth();
			m_nObjHeight = m_image.GetHeight();

			m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, 
				rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			m_image.Destroy();
			setImageStatus();
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchdialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);

		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;		//LBTN
		event.nAction = 1;		//DBCLK
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*) & event, sizeof(event));
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchdialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		TRACE("x = %d, y = %d\r\n", point.x, point.y);
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;		//LBTN
		event.nAction = 2;		//
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

		/*CClientSocket* pClnt = CClientSocket::getInstence();
		CPacket pack(5, (BYTE*)&event, sizeof(event));
		pClnt->Send(pack);*/
	}
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchdialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;		//LBTN
		event.nAction = 3;		//DBCLK
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchdialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;		//RBTN
		event.nAction = 1;		//DBCLK
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchdialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth == -1) && (m_nObjHeight == -1)) {
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;		//RBTN
		event.nAction = 2;		//DOWN
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchdialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;
		event.nAction = 3;
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchdialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 4;
		event.nAction = 4;
		/*CClientSocket* pClnt = CClientSocket::getInstence();
		CPacket pack(5, (BYTE*)&event, sizeof(event));
		pClnt->Send(pack);*/
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchdialog::OnStnClickedWatch()
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint point;
		GetCursorPos(&point);
		// TODO: 在此添加消息处理程序代码和/或调用默认值
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;		//LBTN
		event.nAction = 0; 		//CLCK
		CClntController::getInstance()->SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}
}

void CWatchdialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}


void CWatchdialog::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClntController::getInstance()->SendCommandPacket(7);
}


void CWatchdialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClntController::getInstance()->SendCommandPacket(7);
}
