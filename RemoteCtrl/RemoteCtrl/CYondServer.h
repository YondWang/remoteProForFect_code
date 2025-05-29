#pragma once
#include "CYondThread.h"
#include "CYondQueue.h"
#include "Command.h"
#include <MSWSock.h>
#include <map>

static CCommand m_cmd;
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
	CYondClnt* m_clnt;				//对应的客户端
	WSABUF m_wsabuffer;
	virtual ~YondOverlapped() {
		m_buffer.clear();
		m_wsabuffer.buf = NULL;
		m_wsabuffer.len = 0;
	}

};

template<YondOperator>class AcceptOverlapped;
typedef AcceptOverlapped<YAccept> ACCEPTOVERLAPPED;
template<YondOperator>class RecvOverlapped;
typedef RecvOverlapped<YRecv> RECVOVERLAPPED;
template<YondOperator>class SendOverlapped;
typedef SendOverlapped<YSend> SENDOVERLAPPED;
template<YondOperator>class ErrorOverlapped;
typedef ErrorOverlapped<YError> ERROROVERLAPPED;

class CYondClnt : public ThreadFuncBase {
public:
	CYondClnt();
	~CYondClnt() {
		m_buffer.clear();
		closesocket(m_sock);
		m_recv.reset();
		m_send.reset();
		m_overlapped.reset();
		m_vecSend.Clear();
	}

	void SetOverlaped(CYondClnt* ptr);

	operator SOCKET();
	operator PVOID();
	operator LPOVERLAPPED();
	operator LPDWORD();
	LPWSABUF RecvWSABuffer();
	LPWSAOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPWSAOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLoaclAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize() const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);
public:
	SOCKET m_sock;
	DWORD m_recived;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED>m_recv;
	std::shared_ptr<SENDOVERLAPPED>m_send;
	std::list<CPacket> recvPackets;	//接收数据包列表
	std::list<CPacket> sendPackets;	//发送数据包列表
	std::vector<char> m_buffer;
	size_t m_used;			//已经使用的缓冲区大小
	sockaddr_in m_laddr;	//本地地址
	sockaddr_in m_raddr;	//远程地址
	bool m_isBusy;
	YondSendQueue<std::vector<char>> m_vecSend;		//发送数据队列
};

template<YondOperator>
class AcceptOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();
private:

};

template<YondOperator>
class RecvOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped();
	int RecvtWorker() {
		int index = 0;

		int len = m_clnt->Recv();
		index += len;
		CPacket packet((BYTE*)m_clnt->m_buffer.data(), (size_t&)index);
		m_cmd.ExcuteCommend(packet.sCmd, m_clnt->sendPackets, packet);

		if (index == 0) {
			WSASend((SOCKET)*m_clnt, m_clnt->SendWSABuffer(), 1, *m_clnt, m_clnt->flags(), m_clnt->SendOverlapped(), NULL);
			TRACE("命令：%d\r\n", packet.sCmd);
		}
		return len;
	}
	//~AcceptOverlapped :public YondOverlapped();

private:

};

template<YondOperator>
class SendOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	SendOverlapped();
	int SendWorker() {
		//TODO:
		while (m_clnt->sendPackets.size() > 0) {
			CPacket pack = m_clnt->sendPackets.front();
			m_clnt->sendPackets.pop_front();
			int ret = send(m_clnt->m_sock, pack.Data(), pack.Size(), 0);
			TRACE("send ret： %d\r\n", ret);
		}
		closesocket(m_clnt->m_sock);
		return -1;
	}
	//~AcceptOverlapped :public YondOverlapped();

private:

};

template<YondOperator>
class ErrorOverlapped :public YondOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped() : m_operator(YError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {
		//TODO:
		return -1;
	}
	//~AcceptOverlapped :public YondOverlapped();

private:

};



class CYondServer :
	public ThreadFuncBase
{
public:
	CYondServer(const std::string& ip = "0.0.0.0", short port = 2904);

	~CYondServer();

	bool StartService();
	bool NewAccept();
	void BindNewSocket(SOCKET sock);
private:
	void CreatSocket() {
		WSADATA WSAData;
		if (WSAStartup(MAKEWORD(2, 2), &WSAData)) {
			return;
		}
		m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	
	int threadIocp();
private:
	YondThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<CYondClnt>> m_clnt;
};

