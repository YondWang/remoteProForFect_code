#pragma once
#include "YSocket.h"
#include "CYondThread.h"

class YNetwork
{

};

typedef int (*AcceptFunc)(void* arg, const YSOCKET client);
typedef int (*RecvFunc)(void* arg, const YBuffer& buffer);
typedef int (*SendFunc)(void* arg, YSOCKET& client, int ret); 
typedef int (*RecvFromFunc)(void* arg, const YBuffer& buffer, YSockaddrIn& addr);
typedef int (*SendToFunc)(void* arg, const YSockaddrIn& addr, int ret);


class YServerParamter {
public:
	YServerParamter(const std::string& ip, short port, YTYPE type);
	YServerParamter(const std::string& ip = "0.0.0.0", short port = 2904, YTYPE type = YTYPE::YondTCP,
		AcceptFunc acceptf = NULL, RecvFunc recvf = NULL, SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL, SendToFunc sendtof = NULL);

	//input
	YServerParamter& operator<<(AcceptFunc func);
	YServerParamter& operator<<(RecvFunc func);
	YServerParamter& operator<<(SendFunc func);
	YServerParamter& operator<<(RecvFromFunc func);
	YServerParamter& operator<<(SendToFunc func);
	YServerParamter& operator<<(const std::string& ip);
	YServerParamter& operator<<(short port);
	YServerParamter& operator<<(YTYPE type);
	//output
	YServerParamter& operator>>(AcceptFunc& func);
	YServerParamter& operator>>(RecvFunc& func);
	YServerParamter& operator>>(SendFunc& func);
	YServerParamter& operator>>(RecvFromFunc& func);
	YServerParamter& operator>>(SendToFunc& func);
	YServerParamter& operator>>(std::string& ip);
	YServerParamter& operator>>(short& port);
	YServerParamter& operator>>(YTYPE& type);
	//复制构造函数，等于号重载，用于同类型赋值
	YServerParamter(const YServerParamter& param);
	YServerParamter& operator=(const YServerParamter& param);

	std::string m_ip;
	short		m_port;
	YTYPE		m_type;
	AcceptFunc	m_acceptType;
	RecvFunc	m_recvType;
	SendFunc	m_sendType;
	RecvFromFunc m_recvfrom;
	SendToFunc	m_sendto;
};

class YServer : public ThreadFuncBase
{
public:
	YServer(const YServerParamter& param);		//何时设置关键参数，是根据个人开发经验去调整
	~YServer();
	int Invoke(void* arg);
	int Send(YSOCKET& client, const YBuffer& buffer);
	int Sendto(YSockaddrIn& addr, const YBuffer& buffer);
	int Stop();
private:
	int theradFunc();
	int threadUDPFunc();
	int threadTCPFunc();

private:
	YServerParamter m_param;
	void* m_args;
	CYondThread m_thread;
	YSOCKET m_sock;
	std::atomic<bool> m_stopFlag;
};