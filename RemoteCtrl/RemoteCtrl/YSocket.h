#pragma once
#include <WinSock2.h>
#include <memory>
enum class YTYPE {
	YondTCP = 0,
	YondUDP = 1
};

class YSocket
{
public:
	YSocket(YTYPE nType = YTYPE::YondTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}

	YSocket(const YSocket& sock) {
		m_socket = socket(PF_INET, (int)sock.m_type, m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
	}

	~YSocket() {
		closesocket(m_socket);
	}

	YSocket& operator=(const YSocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
		}
	}
	operator SOCKET() const { return m_socket; }
	operator SOCKET() { return m_socket; }
	bool operator==(SOCKET sock) const {
		return m_socket == sock;
	}

private:
	SOCKET m_socket;
	YTYPE m_type;
	int m_protocol;
};

typedef std::shared_ptr<YSocket> YSOCKET;

class YSockaddrIn {
public:
	YSockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_strIP = inet_ntoa(m_addr.sin_addr);
		m_nPort = ntohs(m_addr.sin_port);
	}
	YSockaddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_strIP = inet_ntoa(m_addr.sin_addr);
	}
	YSockaddrIn(const std::string& strIP, short nPort) {
		m_strIP = strIP;
		m_nPort = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	}
	YSockaddrIn(const YSockaddrIn& addr) {
		if (this != &addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));
		m_strIP = addr.m_strIP;
		m_nPort = addr.m_nPort;
		}
	}

	YSockaddrIn& operator=(const YSockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(addr.m_addr));
			m_nPort = addr.m_nPort;
			m_strIP = addr.m_strIP;
		}
		return *this;
	}
	operator sockaddr* () const { return (sockaddr*)&m_addr; }
	void update() {
		m_strIP = inet_ntoa(m_addr.sin_addr);
		m_nPort = ntohs(m_addr.sin_port);
	}
	std::string GetIP() const { return m_strIP; }
	short GetPort() const { return m_nPort; }

private:
	sockaddr_in m_addr;
	std::string m_strIP;
	short m_nPort;
};