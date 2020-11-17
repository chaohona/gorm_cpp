#ifndef _GORM_CLIENT_THREAD_H__
#define _GORM_CLIENT_THREAD_H__

#include "gorm_thread_pool.h"
#include "gorm_signal_event.h"
#include "gorm_client_msg.h"
#include "gorm_client_event.h"

namespace gorm{


#define GORM_ClientStartStatus_Init 0
#define GORM_ClientStartStatus_Success 1
#define GORM_ClientStartStatus_Failed 2
#define GORM_ClientStartStatus_Vesion_Failed 3

// 客户端收发线程,用于客户端和GORM服务器交互
class GORM_ClientThread: public GORM_Thread
{
public:
    GORM_ClientThread(GORM_Log *logHandle, shared_ptr<GORM_ThreadPool> threadPool);
    virtual ~GORM_ClientThread();

    // 工作线程主循环
    virtual void Work(mutex *m);
    // 工作线程接收到前端请求
    inline int SendRequest(GORM_ClientMsg *request)
    {
        if (GORM_OK != this->clientList[0].SendRequest(request))
        {
            return GORM_ERROR;
        }
        this->NotifyNewRequest();

        return GORM_OK;
    }
    // 线程被唤醒的回调函数
    virtual void SignalCB()
    {
        GORM_ClientEvent &clientEvent = this->clientList[0];
        clientEvent.m_pEpoll->AddEventWrite(&clientEvent);
    }
    inline int GetResponse(GORM_ClientMsg *&reqMsg)
    {
        return this->clientList[0].GetResponse(reqMsg);
    }
    inline int GetStartStatus()
    {
        return this->startStatus;
    }
    void SetStartStatus(int status)
    {
        this->startStatus = status;   
    }
    inline void LoopCheck()
    {
        if (loopIndex % 100 == 0)
        {
            this->clientList[0].LoopCheck();
        }
    }
private:
    int Init();
    // 发送信号，唤醒发送线程
    inline void NotifyNewRequest()
    {
        this->signalEvent->Single();
    }
private:
    GORM_ClientEvent                    *clientList = nullptr;
    shared_ptr<GORM_Epoll>              epoll       = nullptr;
    shared_ptr<GORM_SignalEvent>        signalEvent = nullptr;
    GORM_Log                            *logHandle  = nullptr;
    atomic<int>                         startStatus;
    uint64                              loopIndex = 0;
};

class GORM_ClientThreadPool: public GORM_ThreadPool, 
        public GORM_Singleton<GORM_ClientThreadPool>
{
public:
    GORM_ClientThreadPool();
    virtual ~GORM_ClientThreadPool();

public:
    // TODO 阻塞到线程启动起来
    virtual int CreateThread(GORM_Log *logHandle, int threadNum);

    inline int GetResponse(GORM_ClientMsg *&reqMsg)
    {
        return this->clientThread->GetResponse(reqMsg);
    }
    // 获取和gorm连接的线程启动状态，0为正在建立连接，1为连接成功，2为出现错误
    int ClientThreadStartStatus()
    {
        return this->clientThread->GetStartStatus();
    }
    void LoopCheck()
    {
        if (this->clientThread == nullptr)
            return;
        this->clientThread->LoopCheck();
    }
public:
    // 发送一个请求
    inline int SendRequest(GORM_ClientMsg *request)
    {
        return this->clientThread->SendRequest(request);
    }
private:
    shared_ptr<GORM_ClientThread> clientThread = nullptr;
};

}

#endif

