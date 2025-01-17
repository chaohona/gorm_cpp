#ifndef _GORM_WRAP_H__
#define _GORM_WRAP_H__
#include "gorm_define.h"
#include "gorm_singleton.h"
#include "gorm_log.h"
#include "gorm_sys_inc.h"
#include "gorm_mempool.h"
#include "gorm_queue.h"
#include "gorm_client_msg.h"
#include "gamesh/framework/framework.h"
#include "gamesh/coroutine/coroutine_mgr.h"

namespace gorm{

// 
#define GORM_MAX_CLIENT_REQUEST_POOL 1024*12
// 此类为全局操作类，一些需要全局访问的数据都保存在此类中

#define GORM_RESPONSE_MSG_POOL 1024
class GORM_Wrap: public GORM_Singleton<GORM_Wrap>
{
public:
    GORM_Wrap();
    ~GORM_Wrap();
    // 初始化，程序开始启动的时候调用
    // @param   cfgPath 客户端配置文件
    // @param   logHandle 日志句柄，如果此句柄为空，错误日志会被打印到标准输出上
    // @retval  < 0 初始化失败
    //          = 0 初始化成功
    int Init(const char *cfgPath, GORM_Log *logHandle = nullptr, gamesh::container::Container* container = nullptr);
    // 停止服务
    // @param force 是否强制立即停止，默认为false
    int Stop(bool force = false);
    // 
    // 用于异步请求，获取数据的时候，将数据从收发线程中放到本线程并触发回调函数调用
    // 用于资源回收等工作(在收发线程释放的资源，需要在业务线程回收的工作等)
    // 业务线程可以一帧调用一次
    int OnUpdate(int procNumMax = 1000);

    inline GORM_ClientMsg *GetClientMsg()
    {
        GORM_ClientMsg *tmp = nullptr;
        this->clientMsgPool.Take(tmp);
        return tmp;
    }
    inline void ReleaseClientMsg(GORM_ClientMsg *&msg)
    {
        msg->Reset();
        this->clientMsgPool.Put(msg);
        msg = nullptr; // 强制将外部指针赋值为空，防止外部使用已经释放的数据
    }
public:
    mutex mtx;                                      // 全局锁
    shared_ptr<GORM_MemPool> memPool = nullptr;     // 在业务线程测试用的内存池
    atomic<uint32>   seqIdx;                         // 全局的请求自增下标
        
private:
    // TODO 消息是用内存池
    // ClientMsg消息池子，一共预申请了10万个消息，用完则不动态申请
    GORM_Queue<GORM_ClientMsg*, GORM_MAX_CLIENT_REQUEST_POOL> clientMsgPool;
    GORM_ClientMsg          *msgList = nullptr; // 预分配的消息池子
    GORM_Log                *logHandle = nullptr;
public:
    // zxb 2020-11-10
     gamesh::container::Container* container_ = nullptr;
     gamesh::container::Container* GetContainer()
     {
         return container_;
     }

     // 避免锁操作，从网络线程拷贝到工作线程的，待处理的消息
     GORM_ClientMsg *reponseWaitPorcPool[GORM_RESPONSE_MSG_POOL];
     int64 reponseWaitPorcPoolMaxIndex = 0;
     int64 reponseWaitPorcPoolNowIndex = 0;

     uint64     lastUpdateTime = 0;
     uint64     maxTT = 10;
};


// namespace gorm end
}


#endif
