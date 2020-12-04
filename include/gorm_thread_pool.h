#ifndef _GORM_THREAD_POOL_H__
#define _GORM_THREAD_POOL_H__

#include "gorm_sys_inc.h"
#include "gorm_type.h"
#include "gorm_mempool.h"

namespace gorm{

class GORM_Log;
class GORM_ThreadPool;
class GORM_Thread
{
public:
    GORM_Thread(GORM_Log *logHandle, shared_ptr<GORM_ThreadPool>& pPool);
    virtual ~GORM_Thread();

    void SetLogHandle(GORM_Log* logHandle);
    void SetThreadPool();
    virtual void Work(mutex *m) = 0;
    // 通知事件回调
    // 用于在本线程处理io等待阶段(epoll_wait)，别的线程唤醒本线程的时候的回调
    virtual void SignalCB() = 0;
    virtual void Stop();
    virtual void BeginToUpgrade() = 0;
public:
    virtual void Join();
    virtual void Detach();

public:
    string                  m_strThreadName;
    shared_ptr<thread>      m_pSysThread;
    shared_ptr<GORM_ThreadPool> m_pThreadPool = nullptr;
    atomic_bool             m_bJoined;
    atomic_bool             m_bDetached;
    std::thread::id         m_threadId;
    shared_ptr<GORM_MemPool> m_pMemPool = nullptr;  // 线程自己的内存池
    GORM_Log                *logHandle = nullptr;
    shared_ptr<GORM_Thread> sharedSelf = nullptr;   // stop中置为nullptr，去掉自引用
    int                     stopFlag    = 0;        // 为1则表示外界想让线程结束了
    mutex                   threadMutex;          // 本线程的锁
};

class GORM_ThreadPool
{
public:
    GORM_ThreadPool();
    virtual ~GORM_ThreadPool();

    void SetLogHandle(GORM_Log *logHandle);

    void Stop();
    // 创建iNum个线程
    virtual int CreateThread(GORM_Log *logHandle, int threadNum) = 0;
    // m 为
    void StartWork(shared_ptr<GORM_Thread> pThread);

private:
    void StopThreadGroup();

public:
    mutex m_Mutex;
    string  m_strThreadType;
    unordered_map<std::thread::id, shared_ptr<GORM_Thread>> m_mapThreadGroup;

    GORM_Log *logHandle = nullptr;
private:
    atomic_bool m_bRunning;
    once_flag m_OnceFlag;
};

}
#endif

