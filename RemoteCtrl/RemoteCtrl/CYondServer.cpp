#include "pch.h"
#include "CYondServer.h"
#include "CTool.h"
#pragma warning(disable:4407)

template<YondOperator op>
inline AcceptOverlapped<op>::AcceptOverlapped() {
	m_operator = YAccept;
	m_worker = ThreadWorker(this, (FUNCTYPE) &AcceptOverlapped<op>::AcceptWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template<YondOperator op>
RecvOverlapped<op>::RecvOverlapped() {
	m_operator = YRecv;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped::RecvtWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<YondOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (m_clnt->GetBufferSize() > 0) {
		sockaddr* plocal = NULL, * premote = NULL;
		GetAcceptExSockaddrs(
			*m_clnt, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&plocal, &lLength,
			(sockaddr**)&premote, &rLength);
		memcpy(m_clnt->GetLoaclAddr(), plocal, sizeof(sockaddr_in));
		memcpy(m_clnt->GetRemoteAddr(), premote, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_clnt); //绑定新连接的socket
		int ret = WSARecv((SOCKET)*m_clnt, m_clnt->RecvWSABuffer(), 1, *m_clnt, &m_clnt->flags(), m_clnt->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			TRACE("ret = %d, error = %d\r\n", ret, WSAGetLastError() );
		}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

template<YondOperator op>
SendOverlapped<op>::SendOverlapped() {
	m_operator = YSend;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

CYondClnt::CYondClnt() : 
	m_isBusy(false), m_flags(0) 
	, m_overlapped(new ACCEPTOVERLAPPED()) ,
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_vecSend(this, (SENDCALLBACK)& CYondClnt::SendData)
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void CYondClnt::SetOverlaped(CYondClnt* ptr) {
	m_overlapped->m_clnt = ptr;
	m_recv->m_clnt = ptr;
	m_send->m_clnt = ptr;
}

CYondClnt::operator SOCKET() {
	return m_sock;
}

CYondClnt::operator PVOID() {
	return &m_buffer[0];
}

CYondClnt::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

CYondClnt::operator LPDWORD() {
	return &m_recived;
}

LPWSABUF CYondClnt::RecvWSABuffer() {
	return &m_recv->m_wsabuffer;
}

LPWSAOVERLAPPED CYondClnt::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}

LPWSABUF CYondClnt::SendWSABuffer() {
	return &m_send->m_wsabuffer;
}

LPWSAOVERLAPPED CYondClnt::SendOverlapped()
{
	return &m_send->m_overlapped;
}

int CYondClnt::Recv() {
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0) return -1;
	m_used += (size_t)ret;
	CTool::Dump((BYTE*)m_buffer.data(), ret);
	//TODO:解析数据
	return 0;
}

int CYondClnt::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

int CYondClnt::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_recived, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
			CTool::ShowError();
			return -1;
		}
	}
	return 0;
}

CYondServer::CYondServer(const std::string& ip, short port) : m_pool(10) {
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_sock = INVALID_SOCKET;
	m_addr.sin_family = PF_INET;
	m_addr.sin_port = htons(port);
	m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}

CYondServer::~CYondServer()
{
	closesocket(m_sock);
	std::map<SOCKET, PCLNT>::iterator it = m_clnt.begin();
	for (; it != m_clnt.end(); it++) {
		it->second.reset();
	}
	m_clnt.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool CYondServer::StartService() {
	CreatSocket();

	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		TRACE("bind: %s\r\n", strerror(errno));
		return false;
	}
	if (listen(m_sock, 3) == SOCKET_ERROR) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		TRACE("listen: %s\r\n", strerror(errno));
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	TRACE("server start\r\n");
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		TRACE("CreateIoCompletionPort: %s\r\n", strerror(errno));
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CYondServer::threadIocp));
	if (!NewAccept()) return false;

	return true;
}

bool CYondServer::NewAccept() {
	CYondClnt* pClnt(new CYondClnt());
	pClnt->SetOverlaped(pClnt);
	m_clnt.insert(std::pair<SOCKET, PCLNT>(*pClnt, pClnt));
	if (!AcceptEx(m_sock, *pClnt, *pClnt, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClnt, *pClnt)) {
		TRACE("%d \r\n", WSAGetLastError());
		if (WSAGetLastError() != WSA_IO_PENDING) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void CYondServer::BindNewSocket(SOCKET sock)
{
	CreateIoCompletionPort((HANDLE)sock, m_hIOCP, (ULONG_PTR)this, 0);
}

int CYondServer::threadIocp() {
	DWORD transferred = 0;
	ULONG_PTR completionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &completionKey, &lpOverlapped, INFINITE)) {
		if (completionKey != 0) {
			YondOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, YondOverlapped, m_overlapped);
			TRACE("pOverlapped->m_operator: %p\r\n", pOverlapped->m_operator);
			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator)
			{
			case YAccept:
			{
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
				break;
			}
			case YRecv:
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
				break;
			}
			case YSend:
			{
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
				break;
			}
			case YError:
			{
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
				break;
			}
			}
		}
		else return -1;
	}
	return 0;
}
