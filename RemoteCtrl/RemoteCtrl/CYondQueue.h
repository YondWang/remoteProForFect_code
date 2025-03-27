#pragma once

template<class T>
class CYondQueue
{//线程安全的队列（利用IOCP实现）
public:
	CYondQueue() {}
	~CYondQueue() {}
	void PushBack(const T& data);
	void PopFront(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;

public:
	typedef struct IocpParam {
		int nOperator;
		T strData;
		HANDLE hEvent;	//pop need
		IocpParam(int op, const char* sData) {
			nOperator = op;
			strData = sData;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM;	//Post Parameter 用于投递信息的结构体
	enum {
		YDPush,
		YDPop,
		YDSize,
		YDClear
	};
};
