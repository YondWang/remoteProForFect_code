
// RemoteClntDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "RemoteClnt.h"
#include "RemoteClntDlg.h"
#include "afxdialogex.h"
#include <locale.h>
#include "CWatchdialog.h"
#include "ClntController.h"


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
public:
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
	ON_BN_CLICKED(IDC_BTN_STARTWATCH, &CRemoteClntDlg::OnBnClickedBtnStartwatch)	//WM_COMMAND
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClntDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_port, &CRemoteClntDlg::OnEnChangeEditport)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClntDlg::OnSendPackAck)
END_MESSAGE_MAP()


// CRemoteClntDlg message handlers

void CRemoteClntDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {
	case 1:	//��ȡ������Ϣ
		Str2Tree(strData, m_Tree);
		break;
	case 2:	//��ȡ�ļ���Ϣ
		UpdateFileInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);
		break;
	case 3:
		TRACE("���ļ�����ʼ����\r\n");
		MessageBox("���ļ����!!", "�����ɹ���", MB_ICONINFORMATION);
		break;
	case 4:
		UpdateDownloadFile(strData.c_str(), (FILE*)lParam);
		break;
	case 9:
		TRACE("�ļ�ɾ�����\r\n");
		MessageBox("ɾ���ļ����!!", "�����ɹ���", MB_ICONINFORMATION);
		break;
	case 2001:
		TRACE("�������Ӳ��Գɹ�\r\n");
		MessageBox("���Ӳ��Գɹ�!!", "���ӳɹ���", MB_ICONINFORMATION);
		break;
	default:
		TRACE("�ͻ��˽��յ�δ֪���� %d\r\n", nCmd);
		break;
	}
}

void CRemoteClntDlg::InitUIData()
{
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	UpdateData();
	m_server_address = 0xC0A88B84;	//192.168.139.132
	//m_server_address = 0x7F000001;		//127.0.0.1
	m_nPort = _T(PORT_NUM);
	CClntController* pController = CClntController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
}

void CRemoteClntDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);

	m_List.DeleteAllItems();
	int nCmd = CClntController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();
	while (pInfo->HasNext) {
		TRACE("[%s] is dir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = CClntController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstence()->GetPacket().strData.c_str();
	}
	//CClntController::getInstance()->CloseSocket();
}

void CRemoteClntDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();	//������οؼ��е�������Ŀ
	for (size_t i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {	//�������ŷָ����������ַ���
			dr += ":";
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);	//�����οؼ��в����µĽڵ�
			tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
	if (dr.size() > 0) {
		dr += ":";
		HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);	//�����οؼ��в����µĽڵ�
		tree.InsertItem(NULL, hTemp, TVI_LAST);
	}
}

void CRemoteClntDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParam)
{
	TRACE("hasNext %d, isDirectory %d, szFileName %s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);
	TRACE("hasNext %d, isDirectory %d, szFileName %s\r\n", finfo.HasNext, finfo.IsDirectory, finfo.szFileName);
	if (finfo.HasNext == FALSE)	return;
	if (finfo.IsDirectory) {	//�жϵ�ǰ���Ƿ�ΪĿ¼�����������ؼ��в����½ڵ㣬���ݹ���ú���
		if (CString(finfo.szFileName) == "." || CString(finfo.szFileName) == "..") {	//��������Ŀ¼
			return;
		}
		TRACE("hTreeSelected %08X\r\n", hParam);
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, hParam);	//�����ؼ��в����½ڵ�
		m_Tree.InsertItem("", hTemp, TVI_LAST);	//����һ���սڵ㣬���ڱ�ʾ��Ŀ¼�µ��ļ�
		m_Tree.Expand(hParam, TVE_EXPAND);	//չ���½ڵ�
	}
	else {	//����Ŀ¼����Ϊ�ļ������б�ؼ��в�������
		m_List.InsertItem(0, finfo.szFileName);
	}
}

void CRemoteClntDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	TRACE("length %d index %d \r\n", length, index);
	if (length == 0) {
		length = *(long long*)strData.c_str();
		if (length == 0) {
			AfxMessageBox(_T("�ļ�����Ϊ0���޷���ȡ�ļ�!"));
			CClntController::getInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && (index >= length)) {
		fclose(pFile);
		length = 0;
		index = 0;
		CClntController::getInstance()->DownloadEnd();
	}
	else {
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		TRACE("index = %d\r\n", index);
		if (index >= length) {
			fclose(pFile);
			length = 0;
			index = 0;
			CClntController::getInstance()->DownloadEnd();
		}
	}
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
	// TODO: Add extra initialization here
	InitUIData();
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
	CClntController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2001);
}


void CRemoteClntDlg::OnBnClickedBtnFileinfo()
{
	std::list<CPacket> lstPackets;
	bool ret = CClntController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == 0) {
		AfxMessageBox(_T("�����ʧ�ܣ�����\r\n"));
		return;
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
	CClntController* pClient = CClntController::getInstance();
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	TRACE("hTreeSelected %08X\r\n", hTreeSelected);
	CClntController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath,
		strPath.GetLength(), (WPARAM)hTreeSelected);
}

void CRemoteClntDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClntDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClntDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClntController::getInstance()->DownFile(strFile);
	//����̺߳���
	if (ret != 0) {
		MessageBox(_T("����ʧ�ܣ���"));
		TRACE("����ʧ�� ret = %d\r\n", ret);
	}
}

void CRemoteClntDlg::OnDeletefile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = CClntController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("ɾ���ļ�����ִ��ʧ�ܣ�����");
	}
	AfxMessageBox("ɾ���ɹ�����");
	LoadFileCurrent();
}


void CRemoteClntDlg::OnRunfile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);

	strFile = strPath + strFile;
	int ret = CClntController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("���ļ�����ִ��ʧ�ܣ�����");
	}
}


void CRemoteClntDlg::OnBnClickedBtnStartwatch()
{
	CClntController::getInstance()->StartWatchScreen();
}


void CRemoteClntDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClntDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{

	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	UpdateData();
	CClntController* pController = CClntController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}


void CRemoteClntDlg::OnEnChangeEditport()
{
	UpdateData();
	CClntController* pController = CClntController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}

LRESULT CRemoteClntDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {	//������
		TRACE("socket is error %d \r\n", lParam);
	}
	else if (lParam == 1) {	//һ���ǶԷ��ر����׽���
		TRACE("socket is closed!\r\n");
	}
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(head.sCmd, head.strData, lParam);
		}
	}
	return 0;
}
