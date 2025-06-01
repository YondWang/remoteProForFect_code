#include "YNetwork.h"

YServer::YServer(const YServerParamter& param) : m_stopFlag(false), m_args(NULL){
	m_param = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&YServer::theradFunc));
}

int YServer::Invoke(void* arg)
{
	m_sock.reset(new YSocket(m_param.m_type));
	if (*m_sock == INVALID_SOCKET) {
		printf("%s(%d):%s ERROR!!!!(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return -1;
	}
	if (m_param.m_type == YTYPE::YondTCP) {
		if (m_sock->listen() == -1) {
			return -2;
		}
	}
	YSockaddrIn client;
	if (-1 == m_sock->bind(m_param.m_ip, m_param.m_port)) {
		printf("%s(%d):%s ERROR!!!!(%d)\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return -3;
	}
	if (m_thread.Start() == false)return -4;
	m_args = arg;
	return 0;
}

YServer::~YServer()
{
	Stop();
}

int YServer::Send(YSOCKET& client, const YBuffer& buffer)
{
	int ret = m_sock->send(buffer);		//TODO:优化：发送虽然成功，但不完整
	if (m_param.m_sendType) m_param.m_sendType(m_args, client, ret);
	return ret;
}

int YServer::Sendto(YSockaddrIn& addr, const YBuffer& buffer) {
	int ret = m_sock->sendto(buffer, addr);		//TODO:优化：发送虽然成功，但不完整
	if (m_param.m_sendto)m_param.m_sendto(m_args, addr, ret);
	return ret;
}

int YServer::Stop() {
	if (m_stopFlag == false) {
		m_sock->close();
		m_stopFlag = true;
		m_thread.Stop();
	}
	return 0;
}

int YServer::theradFunc() {
	if (m_param.m_type == YTYPE::YondTCP)
		return threadTCPFunc();
	else
		return threadUDPFunc();
}

int YServer::threadUDPFunc()
{
	YBuffer buf(1024 * 256);
	YSockaddrIn client;
	int ret = 0;
	while (!m_stopFlag) {
		ret = m_sock->recvfrom(buf, client);
		if (ret > 0) {
			client.update();
			if (m_param.m_recvfrom != NULL) {
				m_param.m_recvfrom(m_args, buf, client);
			}
		}
		else {
			printf("%s(%d):%s ERROR(%d)!!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
			break;
		}
	}
	if (m_stopFlag == false) m_stopFlag = true;
	m_sock->close();
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	return 0;
}

int YServer::threadTCPFunc()
{
	return 0;
}

YServerParamter::YServerParamter(const std::string& ip, short port, YTYPE type, 
	AcceptFunc acceptf, RecvFunc recvf, SendFunc sendf, RecvFromFunc recvfromf, 
	SendToFunc sendtof)
{
	m_ip = ip;
	m_port = port;
	m_type = type;
	m_acceptType = acceptf;
	m_recvType = recvf;
	m_sendType = sendf;
	m_recvfrom = recvfromf;
	m_sendto = sendtof;
}

YServerParamter& YServerParamter::operator<<(AcceptFunc func)
{
	m_acceptType = func;
	return *this;
}

YServerParamter& YServerParamter::operator<<(RecvFunc func)
{
	m_recvType = func;
	return *this;
}

YServerParamter& YServerParamter::operator<<(SendFunc func)
{
	m_sendType = func;
	return *this;
}

YServerParamter& YServerParamter::operator<<(RecvFromFunc func)
{
	m_recvfrom = func;
	return *this;
}

YServerParamter& YServerParamter::operator<<(SendToFunc func)
{
	m_sendto = func;
	return *this;
}

YServerParamter& YServerParamter::operator<<(const std::string& ip)
{
	m_ip = ip;
	return *this;
}

YServerParamter& YServerParamter::operator<<(short port)
{
	m_port = port;
	return *this;
}

YServerParamter& YServerParamter::operator<<(YTYPE type)
{
	m_type = type;
	return *this;
}

YServerParamter& YServerParamter::operator>>(AcceptFunc& func)
{
	func = m_acceptType;
	return *this;
}

YServerParamter& YServerParamter::operator>>(RecvFunc& func)
{
	func = m_recvType;
	return *this;
}

YServerParamter& YServerParamter::operator>>(SendFunc& func)
{
	func = m_sendType;
	return *this;
}

YServerParamter& YServerParamter::operator>>(RecvFromFunc& func)
{
	func = m_recvfrom;
	return *this;
}

YServerParamter& YServerParamter::operator>>(SendToFunc& func)
{
	func = m_sendto;
	return *this;
}

YServerParamter& YServerParamter::operator>>(std::string& ip)
{
	ip = m_ip;
	return *this;
}

YServerParamter& YServerParamter::operator>>(short& port)
{
	port = m_port;
	return *this;
}

YServerParamter& YServerParamter::operator>>(YTYPE& type)
{
	type = m_type;
	return *this;
}

YServerParamter::YServerParamter(const YServerParamter& param)
{
	m_ip = param.m_ip;
	m_port = param.m_port;
	m_type = param.m_type;
	m_acceptType = param.m_acceptType;
	m_recvType = param.m_recvType;
	m_sendType = param.m_sendType;
	m_recvfrom = param.m_recvfrom;
	m_sendto = param.m_sendto;
}

YServerParamter& YServerParamter::operator=(const YServerParamter& param)
{
	if (this != &param) {
		m_ip = param.m_ip;
		m_port = param.m_port;
		m_type = param.m_type;
		m_acceptType = param.m_acceptType;
		m_recvType = param.m_recvType;
		m_sendType = param.m_sendType;
		m_recvfrom = param.m_recvfrom;
		m_sendto = param.m_sendto;
	}
	return *this;
	// TODO: 在此处插入 return 语句
}
