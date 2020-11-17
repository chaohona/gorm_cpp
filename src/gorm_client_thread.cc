#include "gorm_client_thread.h"
#include "gorm_client_conf.h"

namespace gorm{

GORM_ClientThread::GORM_ClientThread(GORM_Log *logHandle, shared_ptr<GORM_ThreadPool> threadPool): GORM_Thread(logHandle, threadPool)
{
    this->startStatus = GORM_ClientStartStatus_Init;
    this->clientList = new GORM_ClientEvent[GORM_MAX_CONN_NUM];
}

GORM_ClientThread::~GORM_ClientThread()
{
    if (this->clientList != nullptr)
    {
        delete []clientList;
        this->clientList = nullptr;
    }
}

int GORM_ClientThread::Init()
{
    
    // 创建epoll
    this->epoll = make_shared<GORM_Epoll>();
    if (!this->epoll->Init(1024))
    {
        GORM_CUSTOM_LOGE(logHandle, "gorm epoll init failed.");
        return GORM_ERROR;
    }

    // 创建信号触发器
    this->signalEvent = make_shared<GORM_SignalEvent>(this->epoll, this->sharedSelf);
    this->signalEvent->SetLogHandle(this->logHandle);
    if (GORM_OK != this->signalEvent->Init())
    {
        GORM_CUSTOM_LOGE(logHandle, "init signal event failed.");
        return GORM_ERROR;
    }

    // 和服务器建立连接
    GORM_ClientConfig *clientCfg = GORM_ClientConfig::GetSelfSafe();
    GORM_ServerInfo *svrInfo = clientCfg->GetNextServer();
    clientCfg->connNum = 1; // 目前固定为建立一个连接
    for (int i=0; i<clientCfg->connNum; i++)
    {
        GORM_ClientEvent &clientEvent = this->clientList[i];
        clientEvent.SetThread(this->sharedSelf);
        clientEvent.Init(this->logHandle, this->epoll, this->sharedSelf);
        if (GORM_OK != clientEvent.ConnectToServer(svrInfo->serverAddress.c_str(), svrInfo->serverPort))
        {
            GORM_CUSTOM_LOGE(this->logHandle, "connect to gorm server failed, host:%s, port:%d", svrInfo->serverAddress.c_str(), svrInfo->serverPort);
            return GORM_ERROR;
        }
        this->epoll->AddEventRead(&clientEvent);
    }

    return GORM_OK;
}


void GORM_ClientThread::Work(mutex *m)
{
    if (GORM_OK != this->Init())
    {
        GORM_CUSTOM_LOGE(logHandle, "gorm thread init failed.");
        this->Stop();
        this->startStatus = GORM_ClientStartStatus_Failed; // 启动失败
        return;
    }
    GORM_CUSTOM_LOGI(logHandle, "gorm thread begin to work.");
    //this->startStatus = GORM_ClientStartStatus_Success;  // 启动成功
    // 创建
    for(;stopFlag==0;)
    {
        loopIndex += 1;
        this->epoll->EventLoopProcess(10);
        this->epoll->ProcAllEvents();
        this->LoopCheck();
    }
    this->Stop();
}

GORM_ClientThreadPool::GORM_ClientThreadPool()
{
}

GORM_ClientThreadPool::~GORM_ClientThreadPool()
{
}

int GORM_ClientThreadPool::CreateThread(GORM_Log *logHandle, int threadNum)
{
    threadNum = 1;
    if (this->logHandle == nullptr)
        this->logHandle = GORM_DefaultLog::Instance();
    try
    {
        for (int i=0; i<threadNum; i++)
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            clientThread = make_shared<GORM_ClientThread>(this->logHandle, GORM_ClientThreadPool::SharedPtr());
            this->StartWork(clientThread);
        }
    }
    catch(exception &e)
    {
        GORM_CUSTOM_LOGE(logHandle, "create gorm client thread got exception:%s", e.what());
        return GORM_ERROR;
    }
    
    return GORM_OK;
}


}

