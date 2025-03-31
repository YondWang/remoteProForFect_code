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
inline RecvOverlapped<op>::RecvOverlapped(PCLNT& clnt) : m_operator(YRecv), m_worker(this, &RecvOverlapped::RecvtWorker) {
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
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

template<YondOperator op>
inline SendOverlapped<op>::SendOverlapped() : m_operator(YSend), m_worker(this, &SendOverlapped::SendWorker) {
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<YondOperator op>
inline ErrorOverlapped<op>::ErrorOverlapped() : m_operator(YError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
}



CYondClnt::CYondClnt() : m_isBusy(false), m_overlapped(new ACCEPTOVERLAPPED()) {
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

void CYondClnt::SetOverlaped(PCLNT& ptr) {
	m_overlapped->m_clnt = ptr;
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
