
// RemoteClntDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClnt.h"
#include "RemoteClntDlg.h"
#include "afxdialogex.h"
#include <locale.h>
#include "CWatchdialog.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PORT_NUM "2904"
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
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CRemoteClntDlg::SendCommandPack(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
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
	TRACE("send ret: %d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	if(bAutoClose)
		pClient->CloseSocket();
	return cmd;
}

void CRemoteClntDlg::threadWatchData()
{
	//可能存在异步问题导致程序崩溃
	Sleep(50);
	CClientSocket* pClnt = NULL;
	do {
		pClnt = CClientSocket::getInstence();
	} while (pClnt == NULL);
	ULONGLONG tick = GetTickCount64();
	while(!m_isClosed) {			//= while(true)
		if (m_isFull == false) {//更新数据到缓存
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);		//???
			if (ret == 6) {
					BYTE* pData = (BYTE*)pClnt->GetPacket().strData.c_str();
					//TODO:存入CImage
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
					if (hMem == NULL) {
						TRACE("内存不足！！");
						Sleep(1);
						continue;
					}
					IStream* pStream = NULL;
					HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
					if (hRet == S_OK) {
						ULONG length = 0;
						pStream->Write(pData, pClnt->GetPacket().strData.size(), &length);
						LARGE_INTEGER bg = { 0 };
						pStream->Seek(bg, STREAM_SEEK_SET, NULL);
						if ((HBITMAP)m_image != NULL) m_image.Destroy();
						m_image.Load(pStream);
						m_isFull = true;
					}
				}
			else {
				Sleep(1);
			}
		}
		else Sleep(1);
	}
}

void CRemoteClntDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClntDlg* thiz = (CRemoteClntDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClntDlg::threadEntryDownFile(void* arg)
{
	CRemoteClntDlg* thiz = (CRemoteClntDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClntDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	CFileDialog dlg(FALSE, NULL, strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(_T("本地没有权限保存该文件！或文件无法创建!"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", LPCSTR(strFile));
		CClientSocket* pClnt = CClientSocket::getInstence();
		do {
			//int ret = SendCommandPack(4, false, (BYTE*)LPCSTR(strFile), strFile.GetLength());
			//int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);		//???
			if (ret < 0) {
				AfxMessageBox("执行下载失败!!");
				TRACE("执行下载失败！ ret = %d", ret);
				break;
			}
			long long nLenth = *(long long*)pClnt->GetPacket().strData.c_str();
			if (nLenth == 0) {
				AfxMessageBox("文件长度为0，或无法读取文件！");
				break;
			}
			long long nCount = 0;

			while (nCount < nLenth) {
				ret = pClnt->DealCommand();
				if (ret < 0) {
					AfxMessageBox("传输失败!");
					TRACE("传输失败！ret = %d\r\n", ret);
					break;
				}
				fwrite(pClnt->GetPacket().strData.c_str(), 1, pClnt->GetPacket().strData.size(), pFile);
				nCount += pClnt->GetPacket().strData.size();
			}

		} while (false);
		fclose(pFile);
		pClnt->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成！！"), _T("完成！"));
}

BEGIN_MESSAGE_MAP(CRemoteClntDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClntDlg::OnBnClickedBtnTest)				//WM_COMMAND
	ON_BN_CLICKED(IDC_BTN_FileInfo, &CRemoteClntDlg::OnBnClickedBtnFileinfo)		//WM_COMMAND
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClntDlg::OnNMDblclkTreeDir)			//WM_NOTIFY
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClntDlg::OnNMClickTreeDir)			//WM_NOTIFY
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClntDlg::OnNMRClickListFile)		//WM_NOTIFY
	ON_COMMAND(ID_DOWNLOAD, &CRemoteClntDlg::OnDownloadFile)						//WM_COMMAND
	ON_COMMAND(ID_DELETEFILE, &CRemoteClntDlg::OnDeletefile)						//WM_COMMAND
	ON_COMMAND(ID_RUNFILE, &CRemoteClntDlg::OnRunfile)								//WM_COMMAND
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClntDlg::OnSendPacket)		//注册消息
	ON_BN_CLICKED(IDC_BTN_STARTWATCH, &CRemoteClntDlg::OnBnClickedBtnStartwatch)	//WM_COMMAND
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClntDlg message handlers

void CRemoteClntDlg::LoadFIleCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	
	m_List.DeleteAllItems();
	int nCmd = SendCommandPack(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstence();
	while (pInfo->HasNext) {
		TRACE("[%s] is dir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

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
	//m_server_address = 0xC0A88B84;	//192.168.139.132
	m_server_address = 0x7F000001;		//127.0.0.1
	m_nPort = _T(PORT_NUM);
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;
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
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL , hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (dr.size() > 0) {
		dr += ':';
		HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
		m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
	}
}

CString CRemoteClntDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);

	return strRet;
}

void CRemoteClntDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

void CRemoteClntDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
		return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	int nCmd = SendCommandPack(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstence();
	while (pInfo->HasNext) {
		TRACE("[%s] is dir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if ((CString(pInfo->szFileName) == ".") || (CString(pInfo->szFileName) == "..")) {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem(" ", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();

	}
	//while (true) {
	//	if (!pInfo) break; // 保护性检查，防止野指针

	//	TRACE("Processing file: [%s], is dir: %d, HasNext: %d\r\n",
	//		pInfo->szFileName, pInfo->IsDirectory, pInfo->HasNext);

	//	// 过滤 "." 和 ".."
	//	if (pInfo->IsDirectory && (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..")) {
	//		TRACE("Skipping . and ..\r\n");
	//	}
	//	else {
	//		if (pInfo->IsDirectory) {
	//			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
	//			m_Tree.InsertItem("", hTemp, TVI_LAST);  // 占位符表示可以展开
	//		}
	//		else {
	//			m_List.InsertItem(0, pInfo->szFileName);
	//		}
	//	}

	//	// 检查是否有下一个文件项
	//	if (!pInfo->HasNext) {
	//		TRACE("No more files, requesting next packet...\r\n");
	//		int cmd = pClient->DealCommand();
	//		TRACE("ack: %d\r\n", cmd);
	//		if (cmd < 0) break; // 出错时退出

	//		// 重新获取新包
	//		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	//	}
	//	else {
	//		// 正确递增 pInfo 指针
	//		pInfo = (PFILEINFO)((BYTE*)pInfo + sizeof(FILEINFO));
	//	}
	//}
	pClient->CloseSocket();
}

void CRemoteClntDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClntDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClntDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPumpup = menu.GetSubMenu(0);
	if (pPumpup != NULL) {
		pPumpup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}

}

void CRemoteClntDlg::OnDownloadFile()
{
	//添加线程函数
	_beginthread(CRemoteClntDlg::threadEntryDownFile, 0, this);
	BeginWaitCursor();
	Sleep(50);
	m_dlgStatus.m_info.SetWindowText("命令正在执行中!!");
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}

void CRemoteClntDlg::OnDeletefile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = SendCommandPack(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！！");
	}
	AfxMessageBox("删除成功！！");
	LoadFIleCurrent();
}


void CRemoteClntDlg::OnRunfile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = SendCommandPack(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！！");
	}
}

LRESULT CRemoteClntDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4:	{
		CString strFile = (LPCSTR)lParam;
		ret = SendCommandPack(cmd, wParam & 1, (BYTE*)LPCSTR(strFile), strFile.GetLength());	//定义自定义消息 响应函数
	}
		break;
	case 5: {	//鼠标操作
		ret = SendCommandPack(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
	}
		break;
	case 6:
	case 7:
	case 8: {
		ret = SendCommandPack(cmd, wParam & 1);	//定义自定义消息 响应函数
	}
		break;
	default:
		ret = -1;
		break;
	}
	
	return ret;
}


void CRemoteClntDlg::OnBnClickedBtnStartwatch()
{
	// TODO: 在此添加控件通知处理程序代码
	m_isClosed = false;
	CWatchdialog dlg(this);
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClntDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(hThread, 500);
}


void CRemoteClntDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}
