#pragma once
#include "pch.h"
#include "framework.h"

class CPacket {
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
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
				sHead = *(WORD*)*(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 2 + 4 + 2 > nSize) {	//�����ݿ��ܲ�ȫ�����߰�ͷδ��ȫ���յ�
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {		//��δ��ȫ���յ�������ֱ�ӷ���
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - sCmd - 2 - 2);
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
public:
	WORD sHead;				//�̶�λ FE FF
	DWORD nLength;			//������
	WORD sCmd;				//��������
	std::string strData;	//������
	WORD sSum;				//��У��

};

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
		//TODO:У��
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(2904);
		//TODO:��
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
		if (m_clnt == -1) return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_clnt == -1) return false;
		
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_clnt, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0) {
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
	}
	bool Send(const char* pData, size_t nSize) {
		if (m_clnt == -1) return false;
		return send(m_clnt, pData, nSize, 0) > 0;
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
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
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