#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;
CClientSocket* pserver = CClientSocket::getInstence();

std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

inline bool CClientSocket::InitSocket() {
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) return false;
	//TODO:校验
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	TRACE("addr %08X nIP %08X\r\n", inet_addr("127.0.0.1"), m_nIP);
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);
	TRACE("m_sock = %d\r\n", m_sock);
	if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox(_T("指定的ip地址不存在！！"));
		return false;
	}

	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		AfxMessageBox(_T("连接失败，请检查网络"));
		TRACE("连接失败，%d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}

	return true;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) {
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;
	}
	return ret;
}

//bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,
//	bool isAutoClosed)
//{
//	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
//		//if (InitSocket() == false) return false;
//		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
//		TRACE("start thread\r\n");
//	}
//	PostThreadMessage(m_hThread)
//	m_lock.lock();
//	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
//	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
//	m_lstSend.push_back(pack);
//	m_lock.unlock();
//	WaitForSingleObject(pack.hEvent, INFINITE);
//	std::map<HANDLE, std::list<CPacket>&>::iterator it;
//	it = m_mapAck.find(pack.hEvent);
//	if (it != m_mapAck.end()) {
//		m_lock.lock();
//		m_mapAck.erase(it);
//		m_lock.unlock();
//		return true;
//	}
//	return false;
//}

inline CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

inline CClientSocket::CClientSocket() :
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	m_sock = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("无法初始化套接字环境！检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理线程启动失败\r\n");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, sizeof(m_buffer));
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{ WM_SEND_PACK, &CClientSocket::SendPack },
		{ 0, NULL }
		//{WM_SEND_PACK, },
		
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入失败！消息值：%d 函数值: %08X 序号：%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//TODO:定义一个消息的数据结构(数据和数据长度，模式)  回调消息的数据结构(HWND))
	PACKET_DATA data = *(PACKET_DATA*)wParam; //data是一个局部变量 先释放点，后面使用的时候就不存在内存泄漏的问题啦
	delete (PACKET_DATA*)wParam;//
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0); //存在问题  length为负数
				if (length > 0 || (index > 0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						TRACE("ack pack %d to hWnd %08X %d %d\r\n", pack.sCmd, hWnd, index, nLen);
						TRACE("%04X\r\n", *(WORD*)pBuffer + nLen);
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						//usleep(1);
						if (data.nMode & CSM_AUTOCLOSE) {
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
				}
				else { //TODO:对方关闭了套接字，或者网络设备异常
					TRACE("recv failed length %d index %d cmd %d\r\n", length, index, current.sCmd);
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);
				}
			}
		}
		else {
			CloseSocket();
			//网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else {
		//TODO:错误处理
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}
/*
void CClientSocket::threadFunc()
{
	
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();

	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			TRACE("lst send size:%d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("发送失败");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				do {
					int recv_len = recv(m_sock, pBuffer, BUFFER_SIZE - index, 0);
					TRACE("recv %d %d\r\n", recv_len, index);
					if ((recv_len > 0) || (index > 0)) {
						index += recv_len;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) {	//文件夹信息获取，文件信息可能产生问题
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (recv_len <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);	//等到服务器关闭命令之后，再通知事情完成
						if (it0 != m_mapAutoClosed.end()) {
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
						}
						else {
							TRACE("异常情况，没有对应的pair\r\n");
						}
						break;
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if (InitSocket() == false) InitSocket();
		}
		Sleep(1);
	}
	CloseSocket();
}
*/
void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
