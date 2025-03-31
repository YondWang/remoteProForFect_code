#include "pch.h"
#include "CYondServer.h"
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
	if (*(LPDWORD)*m_clnt.get() > 0) {
		GetAcceptExSockaddrs(
			*m_clnt, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_clnt->GetLoaclAddr(), &lLength,
			(sockaddr**)m_clnt->GetRemoteAddr(), &rLength);
		int ret = WSARecv((SOCKET)*m_clnt, m_clnt->RecvWSABuffer(), 1, *m_clnt, &m_clnt->flags(), *m_clnt, NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			//TODO:±¨´í
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

template<YondOperator op>
ErrorOverlapped<op>::ErrorOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&ErrorOverlapped::ErrorWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
}



CYondClnt::CYondClnt() : 
	m_isBusy(false), m_flags(0) 
	, m_overlapped(new ACCEPTOVERLAPPED()) ,
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED())
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void CYondClnt::SetOverlaped(PCLNT& ptr) {
	m_overlapped->m_clnt = ptr;
	m_recv->m_clnt = ptr;
	m_send->m_clnt = ptr;
}

inline CYondClnt::operator SOCKET() {
	return m_sock;
}

inline CYondClnt::operator PVOID() {
	return &m_buffer[0];
}

inline CYondClnt::operator LPOVERLAPPED() {
	return &m_overlapped->m_overlapped;
}

inline CYondClnt::operator LPDWORD() {
	return &m_recived;
}

inline LPWSABUF CYondClnt::RecvWSABuffer() {
	return &m_recv->m_wsabuffer;
}

inline LPWSABUF CYondClnt::SendWSABuffer() {
	return &m_send->m_wsabuffer;
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

int CYondServer::threadIocp() {
	DWORD transferred = 0;
	ULONG_PTR completionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &completionKey, &lpOverlapped, INFINITE)) {
		if (transferred > 0 && completionKey != 0) {
			YondOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, YondOverlapped, m_overlapped);
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
