#include "gorm_thread_pool.h"
#include "gorm_log.h"

namespace gorm{

GORM_Thread::GORM_Thread(GORM_Log *logHandle, shared_ptr<GORM_ThreadPool>& pPool): m_pThreadPool(pPool), m_bJoined(false), m_bDetached(false)
{
    this->m_pMemPool = make_shared<GORM_MemPool>();
    this->m_pMemPool->m_pMySelf = this->m_pMemPool;
}

GORM_Thread::~GORM_Thread()
{
    this->m_pThreadPool = nullptr;
    this->m_pSysThread = nullptr;
    this->m_pMemPool = nullptr;
    this->sharedSelf = nullptr;
}

void GORM_Thread::SetLogHandle(GORM_Log* logHandle)
{
    this->logHandle = logHandle;
}

void GORM_Thread::Join()
{
    this->m_pSysThread->join();    
}

void GORM_Thread::Detach()
{
    this->m_pSysThread->detach();
}

void GORM_Thread::Stop()
{
    this->sharedSelf = nullptr;
    stopFlag    = 1;
}

GORM_ThreadPool::GORM_ThreadPool()
{
}

GORM_ThreadPool::~GORM_ThreadPool()
{
    Stop();
}

void GORM_ThreadPool::SetLogHandle(GORM_Log *logHandle)
{
    this->logHandle = logHandle;
}

void GORM_ThreadPool::Stop()
{
    call_once(m_OnceFlag, [this]{StopThreadGroup();});
}

void GORM_ThreadPool::StartWork(shared_ptr<GORM_Thread> pThread)
{
    if (pThread == nullptr)
    {
        return;
    }

    pThread->m_pSysThread = make_shared<thread>(&GORM_Thread::Work, pThread, &this->m_Mutex);
    m_mapThreadGroup[pThread->m_pSysThread->get_id()] = pThread;
    pThread->Detach();
    pThread->m_threadId = pThread->m_pSysThread->get_id();
    pThread->sharedSelf = pThread;
    return;
}

void GORM_ThreadPool::StopThreadGroup()
{
    m_bRunning = false;

    for(auto &thread : m_mapThreadGroup)
    {
        thread.second->Stop();
        thread.second->Join();
    }

    m_mapThreadGroup.clear();
}

}
