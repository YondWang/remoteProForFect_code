#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"
#include <vector>
#define PORT_NUM 2904

typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPack);

class CServerSocket
{
public:
	static CServerSocket* getInstence() {
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = PORT_NUM) {
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true) {
			if (AcceptClient() == false) {
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommand();

			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet);
				if (lstPackets.size() > 0) {
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();
		}
		return 0;
	}

#define BUFFER_SIZE 4096000
	int DealCommand() {
		if (m_clnt == -1) return -1;

		char* buffer = m_buffer.data();
		if (buffer == NULL) {
			TRACE("内存不足!!\r\n");
			return -2;
		}
		//memset(buffer, 0, BUFFER_SIZE);
		static size_t index = 0;
		while (1) {
			size_t len = recv(m_clnt, buffer + index, BUFFER_SIZE - index, 0);
			if ((len <= 0) && (index <= 0)) {
				delete[] buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}
	bool Send(const char* pData, size_t nSize) {
		if (m_clnt == -1) return false;
		return send(m_clnt, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_clnt == -1) return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_clnt, pack.Data(), pack.Size(), 0) > 0;
	}
	void CloseClient() {
		if (m_clnt != INVALID_SOCKET) {
			closesocket(m_clnt);
			m_clnt = INVALID_SOCKET;
		}
	}
protected:
	bool InitSocket(short port) {
		if (m_sock == -1) return false;
		//TODO:校验
		struct sockaddr_in serv_addr;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		//TODO:绑定
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
			TRACE("bind: %s\r\n", strerror(errno));
			return false;
		}
		//TODO:
		if (listen(m_sock, 1) == -1) {
			return false;
		}

		return true;
	}
	bool AcceptClient() {
		sockaddr_in clnt_addr;
		int clnt_size = sizeof(clnt_addr);
		m_clnt = accept(m_sock, (sockaddr*)&clnt_addr, &clnt_size);
		TRACE("m_clnt = %d\r\n", m_clnt);
		if (m_clnt == -1) return false;
		return true;
	}
private:
	std::vector <char> m_buffer;
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_clnt, m_sock;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_clnt = ss.m_clnt;
		m_sock = ss.m_sock;
	}

	CServerSocket() {
		m_clnt = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境！检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, sizeof(m_buffer));
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

	bool InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static void realseInstance() {
		if (m_instance != NULL) {
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstence();

		}
		~CHelper() {
			CServerSocket::realseInstance();
		}
	};
	static CHelper m_helper;
};

//extern CServerSocket server;