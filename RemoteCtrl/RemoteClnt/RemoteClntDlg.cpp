
// RemoteClntDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClnt.h"
#include "RemoteClntDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClntDlg dialog



CRemoteClntDlg::CRemoteClntDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLNT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClntDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_port, m_nPort);

	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
}

int CRemoteClntDlg::SendCommandPack(int nCmd, BYTE* pData, size_t nLength)
{
	UpdateData();

	CClientSocket* pClient = CClientSocket::getInstence();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));	//TODO:返回值处理
	if (!ret) {
		AfxMessageBox("网络初始化失败！");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("send ret: %d", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	pClient->CloseSocket();
	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClntDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClntDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FileInfo, &CRemoteClntDlg::OnBnClickedBtnFileinfo)
END_MESSAGE_MAP()


// CRemoteClntDlg message handlers

BOOL CRemoteClntDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("2904");
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRemoteClntDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRemoteClntDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRemoteClntDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteClntDlg::OnBnClickedBtnTest()
{
	SendCommandPack(2001);
}


void CRemoteClntDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPack(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败！！！\r\n"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstence();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++) {		//取得驱动信息
		if (drivers[i] == ',') {
			dr += ":";
			m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
}
