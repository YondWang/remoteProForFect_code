#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"
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
				if (count >= 3) {
					return -2;
				}
				count++;
			}
			int ret = DealCommand();

			if (ret > 0) {
				m_callback(m_arg, ret, lstPackets, m_packet);
				while (ret > 0) {
					if (lstPackets.size() > 0) {
						Send(lstPackets.front());
						lstPackets.pop_front();
					}
				}
			}
			CloseClient();
		}
		return 0;
	}
	
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_clnt == -1) return false;

		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			delete[] buffer;
			TRACE("内存不足!!\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_clnt, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
			TRACE("recv %d\r\n", len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
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
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4) || m_packet.sCmd == 9) {
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {
		return m_packet;
	}
	void CloseClient() {
		if (m_clnt != INVALID_SOCKET) {
			closesocket(m_clnt);
			m_clnt = INVALID_SOCKET;
		}
	}
protected:
	bool InitSocket(short port = PORT_NUM) {
		if (m_sock == -1) return false;
		//TODO:校验
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		//TODO:绑定
		int bind_ret = bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (bind_ret == -1) {
			TRACE("bind:%d %s\r\n", bind_ret, strerror(errno));
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