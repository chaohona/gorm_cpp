#ifndef _GORM_CLIENT_THREAD_H__
#define _GORM_CLIENT_THREAD_H__

#include "gorm_thread_pool.h"
#include "gorm_signal_event.h"
#include "gorm_client_msg.h"
#include "gorm_client_event.h"
#include "gorm_client_conf.h"
#include "gorm_log.h"

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
        int sendRet = this->clientList[request->coid_%this->connNum].SendRequest(request);
        if (GORM_OK != sendRet)
        {
            return sendRet;
        }
		dataFlag = 1;

		this->NotifyNewRequest();

        return GORM_OK;
    }
    // 线程被唤醒的回调函数
    virtual void SignalCB()
    {
        for (int i=0; i<this->connNum; i++)
        {
            GORM_ClientEvent &clientEvent = this->clientList[i];
            if (clientEvent.dataFlag == 1)
            {
                clientEvent.dataFlag = 0;
                clientEvent.m_pEpoll->AddEventWrite(&clientEvent);
            }
        }
    }
    inline int GetResponse(GORM_ClientMsg *&reqMsg)
    {
        reqMsg = nullptr;
        responseMsgList.Take(reqMsg);

        return GORM_OK;
    }
    inline int GetResponse(GORM_ClientMsg **rspPool, int inLen, int64 &num)
    {
        this->responseMsgList.Take(rspPool, inLen, num);

        return GORM_OK;
    }
    inline int GetStartStatus()
    {
        return this->startStatus;
    }
    void SetStartStatus(int status)
    {
        this->startStatus = status;   
    }
    // 工作线程循环检查
    void WorkLoopCheck();
    // 收到服务器要更新重启的命令
    virtual void BeginToUpgrade();
private:
    int Init();
    // 发送信号，唤醒发送线程
    inline void NotifyNewRequest()
    {
        this->signalEvent->Single();
    }
    // 网络线程循环检查
    inline void LoopCheck()
    {
        if (loopIndex % 20 == 0)
        {
            for (int i=0; i<connNum; i++)
            {
                this->clientList[i].NetLoopCheck(loopIndex);
            }
        }
    }
    void ReconnectAllClientWithServer();
private:
    GORM_ClientEvent                    *clientList = nullptr;
    shared_ptr<GORM_Epoll>              epoll       = nullptr;
    shared_ptr<GORM_SignalEvent>        signalEvent = nullptr;
    GORM_Log                            *logHandle  = nullptr;
    atomic<int>                         startStatus;
    uint64                              loopIndex = 0;
	atomic<int> dataFlag;

	atomic<int> upgradeFlag;
	uint64      upgradeTime = 0;
public:
	GORM_SSQueue<GORM_ClientMsg*, GORM_MAX_MUTEX_SENDING_CHANNEL_BUFF>  responseMsgList;    // 获取到响应之后放入此队列，业务线程从这里获取结果
    int connNum = GORM_CONN_NUM;
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
    inline int GetResponse(GORM_ClientMsg **rspPool, int inLen, int64 &num)
    {
        return this->clientThread->GetResponse(rspPool, inLen, num);
    }
    // 获取和gorm连接的线程启动状态，0为正在建立连接，1为连接成功，2为出现错误
    int ClientThreadStartStatus()
    {
        return this->clientThread->GetStartStatus();
    }
    void WorkLoopCheck()
    {
        // 最多十个循环检查一次
        ++this->nowLoopIndex;
        if (this->nowLoopIndex - this->loopCheckIndex < 10)
        {
            return;
        }
        uint64 now = GORM_GetNowMS();
        // 最多500毫秒检查一次
        if (now - this->lastCheckMS < 500)
            return;
        this->lastCheckMS = now;
        this->loopCheckIndex = this->nowLoopIndex;
        this->clientThread->WorkLoopCheck();
    }
public:
    // 发送一个请求
    inline int SendRequest(GORM_ClientMsg *request)
    {
        auto container_ = gorm::GORM_Wrap::Instance()->GetContainer();
        if(!container_) 
        { 
            return -1; 
        } 
        gamesh::coroutine::ICoroutineMgr *comgr = container_->Make<gamesh::coroutine::ICoroutineMgr>().get();
        gamesh::coroutine::CoroutineBase *curco = nullptr;                                            
        if (comgr != nullptr)                                                                           
        {                                                                                               
            curco = comgr->GetCurrentCoroutine();                                                       
        }
        uint64_t cbid = curco->GetInstID(); 
        if (cbid < 0)                       
        {
            GALOG_ERROR("gorm got co id failed.");
            return -1;                      
        }                                   
        request->coid_ = cbid;
     
        
        return this->clientThread->SendRequest(request);
    }

private:
    shared_ptr<GORM_ClientThread> clientThread = nullptr;

    uint64  loopCheckIndex = 0;
    uint64  nowLoopIndex = 0;
    uint64  lastCheckMS = 0;
};

}

#endif

