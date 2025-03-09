#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance = NULL;
CClientSocket::CHelper CClientSocket::m_helper;
CClientSocket* pserver = CClientSocket::getInstence();

std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	if (InitSocket() == false) {
		return;
	}
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("发送失败");
				continue;
			}
			auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>
				(head.hEvent, std::list<CPacket>()));
			int recv_len = recv(m_sock, pBuffer, BUFFER_SIZE - index, 0);
			if (recv_len > 0 || index > 0) {
				index += recv_len;
				size_t size = (size_t)index;
				CPacket pack((BYTE*)pBuffer, size);
				if (size > 0) {	//文件夹信息获取，文件信息可能产生问题
					pack.hEvent = head.hEvent;
					pr.first->second.push_back(pack);
					SetEvent(head.hEvent);
				}
			}
			else if (recv_len <= 0 && index <= 0) {
				CloseSocket();
			}
			m_lstSend.pop_front();
		}
		
	}
}
