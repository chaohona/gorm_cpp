#ifndef _GORM_CLIENT_EVNET_H__
#define _GORM_CLIENT_EVNET_H__

#include "gorm_sys_inc.h"
#include "gorm_event.h"
#include "gorm_log.h"

#include "gorm_client_msg.h"
#include "gorm_ss_queue.h"
#include "gorm_thread_pool.h"
#include "gorm_table_field_map_define.h"
#include "gorm_wrap.h"

namespace gorm{

#define GORM_MAX_SENDING_CHANNEL 1024 // 预分配不超过1024个发送队列,一个线程使用一个发送队列
#define GORM_MAX_SENDING_CHANNEL_BUFF 1024 * 4
#define GORM_MAX_MUTEX_SENDING_CHANNEL_BUFF 1024 * 128 // 12万，达到redis的QPS
#define GORM_EVENT_SENDING_CHANNEL_BUFF  10240

// 处理GORM客户端和GORM服务器的连接
/*
无锁申请资源，资源谁申请谁释放：
1、发送消息机制
需要更新的消息会在业务线程中做好消息封装，然后把需要发送的消息放到发送队列中
当响应回来之后，将响应(以及之前发送的消息)返回给业务线程，并在业务线程中释放组装发送消息申请的内存
2、响应消息的处理
在请求的时候先申请1K的缓存用于接收响应消息(大部分情况都是够用的)，以达到谁使用谁申请谁释放的目的
*/

// 1、待发送的消息先在发送对列中
// 2、消息发送出去之后，会被放到响应对列中
// 3、获取到响应之后从响应队列中获取对应的请求数据
class GORM_ClientEvent : public GORM_Event
{
public:
    GORM_ClientEvent();
    virtual ~GORM_ClientEvent();

    // 用于按数组分配无法显示调用带参数构造函数场景
    void Init(GORM_Log *logHandle, shared_ptr<GORM_Epoll> epoll, shared_ptr<GORM_Thread>& thread);

    // 连接GORM服务器，阻塞调用
    int ConnectToServer(const char *host, uint16 port);

    void SetThread(shared_ptr<GORM_Thread>& thread);

    // 优先发送请求
    inline int PreSendRequest(GORM_ClientMsg *request)
    {
        // 如果已经断开连接了，则失败
        // TODO 如果业务层是单线程，这个锁不需要
        uint64 now = GORM_GetNowMS();
        request->requestTMS = now;
        if (!this->waitSendingChannel.PutPre(request))
            return GORM_NO_MORE_SEND_CHANNEL;

        this->dataFlag = 1;
        return GORM_OK;
    }

    // 发送一个请求
    inline int SendRequest(GORM_ClientMsg *request)
    {
        // 如果已经断开连接了，则失败
        // TODO 如果业务层是单线程，这个锁不需要
        uint64 now = GORM_GetNowMS();
        request->requestTMS = now;
        if (!this->waitSendingChannel.Put(request))
            return GORM_NO_MORE_SEND_CHANNEL;

        this->dataFlag = 1;
        return GORM_OK;
    }
    // 网络线程循环检查
    void NetLoopCheck(uint64 loopIndex);
    // 业务线程调用的检查
    void WorkLoopCheck();
    virtual int Close();
public:
    virtual int Write();
    virtual int Read();

private:
    // 获取下一条需要被发送的消息
    inline int GetNextSendingRequest()
    {
        if (this->sendingRequest != nullptr)
        {
            return GORM_OK;
        }
        if (this->waitSendingChannel.Take(this->sendingRequest) && this->sendingRequest != nullptr)
        {
            this->sendingPos = this->sendingRequest->reqMemData->m_uszData;
            if (this->sendingRequest->needCBFlag == GORM_REQUEST_NEED_CB)
                clientWaitRspMsgMap[this->sendingRequest->cbId] = this->sendingRequest;
            return GORM_OK;
        }
        
        return GORM_OK;
    }
    // 发送完一条消息的后处理
    inline int OneRequestFinishSend()
    {
        uint64 now = GORM_GetNowMS();
        GALOG_DEBUG("gorm, sending tt:%llu", now - this->sendingRequest->requestTMS);
        this->send2ServerNum += 1;
        this->sendingRequest = nullptr;
        
        return GORM_OK;
    }
    // 开始处理一条新的消息
    int BeginReadNextMsg();
    // 读取到至少一条完整消息了，开始解析响应
    int OneRspFinishRead();
    inline int GetReqMsgForResponse(uint32 cbId, GORM_ClientMsg *&reqMsg)
    {
        reqMsg = clientWaitRspMsgMap[cbId];
        if (reqMsg != nullptr)
            this->clientWaitRspMsgMap.erase(cbId);
        return GORM_OK;
    }
    int PutResponseToList(uint32 cbId, GORM_ClientMsg *reqMsg);
    int SendHandShakeMsg();
    int HandShakeResult(char preErrCode);
	int Reconnect(const char *szIP, uint16 uiPort);
	int DoAfterReconnect(); // 重连之后的后处理

	void BeginToUpgrade();
public:
    mutex           mtx;                        // 用于互斥获取发送队列id等操作时使用
    list<int>       freeChannelId;              // 申请之后释放的发送队列id
    atomic<int>     sendChannelUsedIdx;         // 发送队列使用下标

    atomic<int>     pendingSendMsgNum;
    atomic<uint32>  cbIdSeedAtomic;
    int64           cbIdSeed = 0;
    
    GORM_SSQueue<GORM_ClientMsg*, GORM_EVENT_SENDING_CHANNEL_BUFF>  waitSendingChannel; // 等待发送的队列
    uint64              send2ServerNum = 0;
    GORM_ClientMsg      *sendingRequest = nullptr;  // 正在发送的消息
    int                 sendingChanneldId = -1;     // 正在发送的消息所在的channel
    // 发送消息的下标
    char *sendingPos = nullptr;
    ////////////////响应处理相关数据
    unordered_map<uint32, GORM_ClientMsg*>  clientWaitRspMsgMap;                            // 只有发送线程会使用    
    //
    GORM_MemPoolData                *readingBuffer = nullptr;   // 读取GORMSERVER响应缓冲区
    char                            *readPos = nullptr;         // 当前读缓冲区已使用到的位置
    char                            *beginReadPos = nullptr;    // 读缓冲区开始位置
    int                             needReadLen = 0;            // 当前消息需要读取的长度
    shared_ptr<GORM_Thread>         myThread = nullptr;         // 当前工作的线程

    int upgradeFlag = 0;
    uint64 upgradeMS = 0;
public:
    atomic<int> dataFlag;
};

}

#endif
