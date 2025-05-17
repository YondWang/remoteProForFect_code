#pragma once
#include <atomic>
#include "pch.h"
#include <list>
#include "CYondThread.h"

template<class T>
class CYondQueue
{//线程安全的队列（利用IOCP实现）
public:
	enum {
		YDNone,
		YDPush,
		YDPop,
		YDSize,
		YDClear
	};
	typedef struct IocpParam {
		size_t nOperator;
		T Data;
		HANDLE hEvent;	//pop need
		IocpParam(int op, const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = YDNone;
		}
	}PPARAM;	//Post Parameter 用于投递信息的结构体

public:
	CYondQueue() {
		m_atom = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(
				&CYondQueue<T>::threadEntry,
				0, this);
		}
	}
	~CYondQueue() {
		if (m_atom) return;
		m_atom = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);

		}
		//m_lstData.clear();
	}
	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(YDPush, data);
		if (m_atom) {
			delete pParam;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}
	virtual bool PopFront(T& data) {
		if (m_atom) return false;
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(YDPop, data, hEvent);
		if (m_atom) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(YDSize, T(), hEvent);
		if (m_atom) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		if (m_atom) return false;
		IocpParam* pParam = new IocpParam(YDClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;
		return ret;
	}
protected:
	static void threadEntry(void* arg) {
		CYondQueue<T>* thiz = (CYondQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
		case YDPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case YDPop:
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
			break;
		case YDSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
			break;
		case YDClear:
			m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugString(_T("unknown operation!\r\n"));
			break;
		}
	}
	virtual void threadMain() {
		PPARAM* pParam = NULL;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		DWORD dwTransferred = 0;

		while (GetQueuedCompletionStatus(m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, INFINITE)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				TRACE("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompeletionPort,
			&dwTransferred,
			&CompletionKey,
			&pOverlapped, 0)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				TRACE("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_atom;		//队列正在析构

};


template<class T>
class YondSendQueue : public CYondQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* YDCALLBACK)(T& data);

	YondSendQueue(ThreadFuncBase* obj, YDCALLBACK callback) :
		CYondQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&YondSendQueue<T>::threadTick));

	}

protected:
	virtual bool PopFront(T& data) {
		return false;
	}
	bool PopFront()
	{
		if (CYondQueue<T>::m_atom) return false;
		typename CYondQueue<T>::IocpParam* Param = new typename CYondQueue<T>::IocpParam(CYondQueue<T>::YDPop, T());
		if (CYondQueue<T>::m_atom) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CYondQueue<T>::m_hCompeletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}
	int threadTick() {
		if (CYondQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		Sleep(1);
		return 0;
	}
	void DealParam(typename CYondQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator) {
		case CYondQueue<T>::YDPush:
			CYondQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case CYondQueue<T>::YDPop:
			if (CYondQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CYondQueue<T>::m_lstData.front();
				if ((m_base->*m_callback())(pParam->Data) == 0)
					CYondQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
			break;
		case CYondQueue<T>::YDSize:
			pParam->nOperator = CYondQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
			break;
		case CYondQueue<T>::YDClear:
			CYondQueue<T>::m_lstData.clear();
			delete pParam;
			break;
		default:
			OutputDebugString(_T("unknown operation!\r\n"));
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	YDCALLBACK m_callback;
	CYondThread m_thread;
};
typedef YondSendQueue<std::vector<char>>::YDCALLBACK SENDCALLBACK;