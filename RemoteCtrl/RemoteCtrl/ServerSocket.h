#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket {
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 2 + 4 + 2 > nSize) {	//包数据可能不全，或者包头未能全部收到
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {		//包未完全接收到，于是直接返回
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;	//head length data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	CPacket operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}

	int Size() {	//包数据大小
		return nLength + 6;
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)(pData) = sHead; pData += 2;
		*(DWORD*)(pData) = nLength; pData += 4;
		*(WORD*)(pData) = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead;				//固定位 FE FF
	DWORD nLength;			//包长度
	WORD sCmd;				//控制命令
	std::string strData;	//包数据
	WORD sSum;				//和校验
	std::string strOut;		//整个包的数据

};
#pragma pack(pop)
typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//点击、移动、双击
	WORD nButton;	//left,right button
	POINT ptXY;		//坐标
}MOUSEEV, *PMOUSEEV;

class CServerSocket
{
public:
	static CServerSocket* getInstence() {
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	bool InitSocket() {
		if (m_sock == -1) return false;
		//TODO:校验
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(2904);
		//TODO:绑定
		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr))) {
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
		size_t index = 0;
		while (true) {
			size_t len = recv(m_clnt, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				delete[] buffer;
				return -1;
			}
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
		return send(m_clnt, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& strPath) {
		if (m_packet.sCmd >= 2 && m_packet.sCmd <= 3) {
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
		closesocket(m_clnt);
		m_clnt = INVALID_SOCKET;
	}
private:
	SOCKET m_clnt, m_sock;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_clnt = ss.m_clnt;
		m_sock = ss.m_sock;
	}

	CServerSocket() {
		m_clnt = INVALID_SOCKET;
		if(InitSockEnv() == FALSE) {
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

extern CServerSocket server;