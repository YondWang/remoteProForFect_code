#pragma once
#include "CYondThread.h"
#include "CYondQueue.h"
#include <MSWSock.h>
#include <map>
#include <list>

class CYondServer;
class CYondClnt;

typedef std::shared_ptr<CYondClnt> PCLNT;

enum YondOperator {
	YNone,
	YAccept,
	YRecv,
	YSend,
	YError
};

class YondOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;	//操作类型 参见YondOperator
	std::vector<char> m_buffer;	//缓冲区
	ThreadWorker m_worker;		//处理函数
	CYondServer* m_server;		//服务器指针
};

template<YondOperator>
class AcceptOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();
	//~AcceptOverlapped :public YondOverlapped();
	PCLNT m_clnt;
private:

};
typedef AcceptOverlapped<YAccept> ACCEPTOVERLAPPED;

template<YondOperator>
class RecvOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped(PCLNT& clnt);
	int RecvtWorker() {
		//TODO:
	}
	//~AcceptOverlapped :public YondOverlapped();

private:

};
typedef RecvOverlapped<YRecv> RECVOVERLAPPED;

template<YondOperator>
class SendOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	SendOverlapped();
	int SendWorker() {
		//TODO:
	}
	//~AcceptOverlapped :public YondOverlapped();

private:

};
typedef SendOverlapped<YSend> SENDOVERLAPPED;

template<YondOperator>
class ErrorOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped();
	int ErrorWorker() {
		//TODO:
	}
	//~AcceptOverlapped :public YondOverlapped();

private:

};
typedef ErrorOverlapped<YError> ERROROVERLAPPED;

class CYondClnt {
public:
	CYondClnt();
	~CYondClnt() {
		closesocket(m_sock);
	}

	void SetOverlaped(PCLNT& ptr);

	operator SOCKET();
	operator PVOID();
	operator LPOVERLAPPED();
	operator LPDWORD();
	sockaddr_in* GetLoaclAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
private:
	SOCKET m_sock;
	DWORD m_recived;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr;	//本地地址
	sockaddr_in m_raddr;	//远程地址
	bool m_isBusy;
};

class CYondServer :
	public ThreadFuncBase
{
public:
	CYondServer(const std::string& ip = "0.0.0.0", short port = 2904) : m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = PF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}

	~CYondServer() {}
	bool StartService() {
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
	bool NewAccept() {
		PCLNT pClnt(new CYondClnt());
		pClnt->SetOverlaped(pClnt);
		m_clnt.insert(std::pair<SOCKET, PCLNT>(*pClnt, pClnt));
		if (!AcceptEx(
			m_sock, *pClnt, *pClnt, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClnt, *pClnt)) {
			return false;
		}
		return true;
	}
private:
	void CreatSocket() {
		m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	
	int threadIocp() {
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
private:
	YondThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<CYondClnt>> m_clnt;
};

