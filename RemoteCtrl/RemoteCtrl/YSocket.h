#pragma once
#include <WinSock2.h>
#include <memory>
#include <string>
enum class YTYPE {
	YondTCP = 0,
	YondUDP = 1
};

class YSocket;

class YSockaddrIn {
public:
	YSockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_nPort = -1;
	}
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
	operator void* () const { return (void*)&m_addr; }
	operator sockaddr* () const { return (sockaddr*)&m_addr; }
	void update() {
		m_strIP = inet_ntoa(m_addr.sin_addr);
		m_nPort = ntohs(m_addr.sin_port);
	}
	std::string GetIP() const { return m_strIP; }
	short GetPort() const { return m_nPort; }
	inline int size() const { return sizeof(sockaddr_in); }

private:
	sockaddr_in m_addr;
	std::string m_strIP;
	short m_nPort;
};

class YBuffer : public std::string {
public:
	YBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	YBuffer(size_t size = 0) :std::string() {
		if (size > 0) {
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	YBuffer(void* buffer, size_t size) : std::string(){
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~YBuffer() {
		//std::string::~basic_string();
	}
	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }
	void update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

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
		m_addrIn = sock.m_addrIn;
	}

	~YSocket() {
		close();
	}

	YSocket& operator=(const YSocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
			m_addrIn = sock.m_addrIn;
		}
	}
	operator SOCKET() const { return m_socket; }
	operator SOCKET() { return m_socket; }
	bool operator==(SOCKET sock) const {
		return m_socket == sock;
	}
	int listen(int backlog = 5) {
		if (m_type != YTYPE::YondTCP) return -1;
		return ::listen(m_socket, backlog);
	}
	int bind(const std::string& ip, short port) {
		m_addrIn = YSockaddrIn(ip, port);
		return ::bind(m_socket, m_addrIn, m_addrIn.size());
	}
	int accept() {}
	int connect(const std::string& ip, short port) {}
	int send(const YBuffer& buffer) {
		return ::send(m_socket, buffer, buffer.size(), 0);
	}
	int recv(YBuffer& buffer) {
		return ::recv(m_socket, buffer, buffer.size(), 0);
	}
	int sendto(const YBuffer& buffer, const YSockaddrIn& to) {
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.size());
	}
	int recvfrom(YBuffer& buffer, YSockaddrIn& from) {
		int len = from.size();
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);
		if (ret > 0) {
			from.update();
		}
		return ret;
	}
	void close() {
		if (m_socket == INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
	

private:
	SOCKET m_socket;
	YTYPE m_type;
	int m_protocol;
	YSockaddrIn m_addrIn;
};

typedef std::shared_ptr<YSocket> YSOCKET;
